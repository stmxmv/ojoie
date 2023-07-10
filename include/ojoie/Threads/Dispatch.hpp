//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_DISPATCH_HPP
#define OJOIE_DISPATCH_HPP

#include "Threads.hpp"
#include <ojoie/Template/delegate.hpp>
#include <ojoie/Threads/Task.hpp>

namespace AN {

namespace Dispatch {

enum Kind {
    Main,
    Game,
    Render,

    Num
};


AN_API Delegate<void(TaskInterface)> *GetDelegate();

AN_API ThreadID GetThreadID(Kind kind);

AN_API bool IsRunning(Kind kind);

AN_API void SetThreadID(Kind kind, ThreadID id);

/// \attention notice that only game thread can dispatch to render thread
template<typename Func>
void async(Kind kind, Func &&task) {
    GetDelegate()[(int)kind](TaskItem(std::forward<Func>(task)));
}


}




}

#endif//OJOIE_DISPATCH_HPP
