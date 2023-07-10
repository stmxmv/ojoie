//
// Created by Aleudillonam on 7/27/2022.
//

#include "Render/RenderQueue.hpp"
#include "Threads/Dispatch.hpp"

#include "Utility/Log.h"
#include <concurrentqueue/concurrentqueue.hpp>


namespace AN {



struct RenderQueue::Impl {
    moodycamel::ConcurrentQueue<TaskInterface> taskQueue;
};


RenderQueue::RenderQueue() : impl(new Impl()), isStop() {
    Dispatch::GetDelegate()[Dispatch::Render].bind(this, &RenderQueue::__enqueue);
}

RenderQueue::~RenderQueue() {
    stop();
    if (renderThread.joinable()) {
        renderThread.join();
    }
    delete impl;
}

bool RenderQueue::init() {
    Dispatch::SetThreadID(Dispatch::Render, GetCurrentThreadID());
    return true;
    renderThread = std::thread([this]{
        Dispatch::SetThreadID(Dispatch::Render, GetCurrentThreadID());
        SetCurrentThreadName("com.an.RenderThread");

        AN_LOG(Log, "render thread start");

        moodycamel::ConsumerToken token(impl->taskQueue);

        for (;;) {


            if (taskNum.fetch_sub(1, std::memory_order_acquire) == 0) [[unlikely]] {
                if (isStop) {
                    break;
                }
                taskNum.wait(-1, std::memory_order_acquire);
            }
            TaskInterface task;
            if (impl->taskQueue.try_dequeue(token, task)) {
                task.run();
            } else {
                taskNum.fetch_add(1, std::memory_order_relaxed);
            }

        }

        while (!cleanupTasks.empty()) {
            cleanupTasks.top().run();
            cleanupTasks.pop();
        }

    });
    return true;
}


void RenderQueue::__enqueue(TaskInterface task) {
    task.run();
//    impl->taskQueue.enqueue(task);
//    if (taskNum.fetch_add(1, std::memory_order_release) == -1) {
//        taskNum.notify_one();
//    }
}

void RenderQueue::stop() {
    if (!isStop) {
        isStop = true;
        enqueue([]{});
    }
}

void RenderQueue::stopAndWait() {
    stop();
//    renderThread.join();
}


RenderQueue &RenderQueue::GetSharedRenderQueue() {
    static RenderQueue renderQueue;
    return renderQueue;
}



}