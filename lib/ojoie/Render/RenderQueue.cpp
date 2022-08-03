//
// Created by Aleudillonam on 7/27/2022.
//

#include "Render/RenderQueue.hpp"
#include "Core/DispatchQueue.hpp"

#include <readerwriterqueue/readerwriterqueue.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#endif

namespace AN {



struct RenderQueue::Impl {
    moodycamel::ReaderWriterQueue<TaskInterface> taskQueue;
};


RenderQueue::RenderQueue() : impl(new Impl()), isStop() {
    DispatchQueue::GetDelegate()[DispatchQueue::Render].bind(this, &RenderQueue::__enqueue);
}

RenderQueue::~RenderQueue() {
    stop();
    if (renderThread.joinable()) {
        renderThread.join();
    }
    delete impl;
}

bool RenderQueue::init() {
    renderThread = std::thread([this]{

        DispatchQueue::SetThreadID(DispatchQueue::Render, std::this_thread::get_id());
#ifdef _WIN32
        SetThreadDescription(GetCurrentThread(),L"com.an.RenderThread");
#endif

        for (;;) {

            if (taskNum.fetch_sub(1, std::memory_order_acquire) == 0) [[unlikely]] {
                if (isStop) {
                    break;
                }
                taskNum.wait(-1, std::memory_order_acquire);
            }
            TaskInterface task;
            if (impl->taskQueue.try_dequeue(task)) [[likely]] {
                task.run();
            } else {
                taskNum.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });
    return true;
}


void RenderQueue::__enqueue(const TaskInterface &task) {
    impl->taskQueue.enqueue(task);
    if (taskNum.fetch_add(1, std::memory_order_release) == -1) {
        taskNum.notify_one();
    }
}

void RenderQueue::stop() {
    if (!isStop) {
        isStop = true;
        enqueue([]{});
    }
}

void RenderQueue::stopAndWait() {
    stop();
    renderThread.join();
}


RenderQueue &RenderQueue::GetSharedRenderQueue() {
    static RenderQueue renderQueue;
    return renderQueue;
}



}