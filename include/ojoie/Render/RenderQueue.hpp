//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_RENDERQUEUE_HPP
#define OJOIE_RENDERQUEUE_HPP

#include <ojoie/Core/SpinLock.hpp>
#include <ojoie/Core/Task.hpp>
#include <ojoie/Core/delegate.hpp>
#include <stack>
#include <thread>


namespace AN {


class RenderQueue {

    struct Impl;
    Impl *impl;
    std::thread renderThread;
    std::atomic_int taskNum;
    std::atomic_bool isStop;

    SpinLock selfLock;
    std::stack<TaskInterface> cleanupTasks;

    void __enqueue(const TaskInterface &task);

    RenderQueue();

    ~RenderQueue();

    friend class Renderer;
public:

    static RenderQueue &GetSharedRenderQueue();

    bool init();

    void stop();

    void stopAndWait();

    /// \GameActor
    template<typename Func>
    void enqueue(Func &&func) {
        __enqueue(TaskItem(std::forward<Func>(func)));
    }

    bool isRunning() const {
        return !isStop;
    }

    /// \AnyActor
    template<typename Func>
    void registerCleanupTask(Func &&func) {
        std::lock_guard lock(selfLock);
        cleanupTasks.push(TaskItem(std::forward<Func>(func)));
    }

};


inline RenderQueue &GetRenderQueue() {
    return RenderQueue::GetSharedRenderQueue();
}



class RenderFence {
    std::atomic_flag flag;
public:

    RenderFence() {
        GetRenderQueue().enqueue([this]{
            flag.test_and_set(std::memory_order_release);
            flag.notify_one();
        });
    }

    void wait() {
        flag.wait(false, std::memory_order_acquire);
    }
};


}

#endif//OJOIE_RENDERQUEUE_HPP
