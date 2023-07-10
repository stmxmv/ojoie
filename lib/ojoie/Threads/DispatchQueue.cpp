//
// Created by Aleudillonam on 7/28/2022.
//

#include "Threads/Dispatch.hpp"

namespace AN::Dispatch {

Delegate<void(TaskInterface )> *GetDelegate() {
    static Delegate<void(TaskInterface)> delegates[(int)Num];
    return delegates;
}

static ThreadID threadIDs[Num] = {};

ThreadID GetThreadID(Kind kind) {
    return threadIDs[kind];
}

void SetThreadID(Kind kind, ThreadID id) {
    threadIDs[kind] = id;
}

bool IsRunning(Kind kind) {
    return threadIDs[kind];
}

}