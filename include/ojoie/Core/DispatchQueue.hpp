//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_DISPATCHQUEUE_HPP
#define OJOIE_DISPATCHQUEUE_HPP

#include <ojoie/Core/Task.hpp>
#include <ojoie/Core/delegate.hpp>

#include <thread>

namespace AN {

namespace DispatchQueue {

enum Kind {
    Main,
    Game,
    Render,

    Num
};

Delegate<void(const TaskInterface&)> *GetDelegate();

std::thread::id GetThreadID(Kind kind);

bool isRunning(Kind kind);

void SetThreadID(Kind kind, std::thread::id id);

/// \attention notice that only game thread can dispatch to render thread
template<typename Func>
void async(Kind kind, Func &&task) {
    GetDelegate()[(int)kind](TaskItem(std::forward<Func>(task)));
}


}




}

#endif//OJOIE_DISPATCHQUEUE_HPP
