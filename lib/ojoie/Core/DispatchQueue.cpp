//
// Created by Aleudillonam on 7/28/2022.
//

#include "Core/Dispatch.hpp"

namespace AN::Dispatch {



Delegate<void(const TaskInterface &)> *GetDelegate() {
    static Delegate<void(const TaskInterface&)> delegates[(int)Num];
    return delegates;
}

static std::thread::id threadIDs[Num] = {};

std::thread::id GetThreadID(Kind kind) {
    return threadIDs[kind];
}

void SetThreadID(Kind kind, std::thread::id id) {
    threadIDs[kind] = id;
}

bool isRunning(Kind kind) {
    return *(uint64_t *)&threadIDs[kind];
}

}