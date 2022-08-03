//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_RENDERQUEUE_HPP
#define OJOIE_RENDERQUEUE_HPP

#include <ojoie/Core/Task.hpp>
#include <ojoie/Core/delegate.hpp>
#include <thread>



namespace AN {


class RenderQueue {

    struct Impl;
    Impl *impl;
    std::thread renderThread;
    std::atomic_int taskNum;
    volatile bool isStop;

    void __enqueue(const TaskInterface &task);

    RenderQueue();

    ~RenderQueue();

    friend class Renderer;
public:

    static RenderQueue &GetSharedRenderQueue();

    bool init();

    void stop();

    void stopAndWait();

    /// \attention not thread safe
    template<typename Func>
    void enqueue(Func &&func) {
        __enqueue(TaskItem(std::forward<Func>(func)));
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
            flag.test_and_set();
            flag.notify_one();
        });
    }

    void wait() {
        flag.wait(false);
    }
};


}

#endif//OJOIE_RENDERQUEUE_HPP
