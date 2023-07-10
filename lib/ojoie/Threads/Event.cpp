//
// Created by Aleudillonam on 7/30/2022.
//

#include "Threads/Event.hpp"


#ifdef _WIN32
#include <Windows.h>
#endif //_WIN32

namespace AN {


#ifdef _WIN32

Event::Event() : pimpl() {} //NOLINT

Event::~Event() {
    if (pimpl != nullptr) {
        CloseHandle(pimpl);
    }
}

bool Event::init(bool bIsManualReset) {
    // Create the event and default it to non-signaled
    pimpl = (struct Pimpl *)CreateEvent(nullptr, bIsManualReset, 0, nullptr);
    ManualReset = bIsManualReset;
    return pimpl != nullptr;
}

void Event::trigger() {
    SetEvent(pimpl);
}

void Event::reset() {
    ResetEvent(pimpl);
}

bool Event::wait(int WaitTime) {
    return WaitForSingleObject(pimpl, WaitTime) == WAIT_OBJECT_0;
}


#else



Event::Event() : inited() {}

Event::~Event() {
    if (inited) {
        {
            std::lock_guard lock(mutex);
            ManualReset = true;
        }
        trigger();

        std::unique_lock lock(mutex);

        while (waitingThreads) {
            lock.unlock();
            std::this_thread::yield();
            lock.lock();
        }

    }
}

bool Event::init(bool bIsManualReset /* = false*/) {
    assert(!inited);
    ManualReset = bIsManualReset;
    triggered = TriggerType::none;
    waitingThreads = 0;
    inited = true;
    return true;
}

void Event::trigger() {
    assert(inited);
    if (ManualReset) {
        {
            std::lock_guard lockGuard(mutex);
            triggered = TriggerType::all;
        }
        cond.notify_all();
    } else {
        {
            std::lock_guard lockGuard(mutex);
            triggered = TriggerType::one;
        }
        cond.notify_one();
    }
}

void Event::reset() {
    assert(inited);
    std::lock_guard lockGuard(mutex);
    triggered = TriggerType::none;
}

bool Event::wait(int WaitTime) {
    assert(inited);
    bool ret = false;
    std::unique_lock lock(mutex);
    waitingThreads = waitingThreads + 1;
    cond.wait_for(lock,std::chrono::milliseconds(WaitTime),
                  [this, &ret]{

                      if (triggered == TriggerType::one) {
                          triggered = TriggerType::none;
                          ret = true;
                          return true;
                      }
                      if (triggered == TriggerType::all) {
                          ret = true;
                          return true;
                      }

                      return false;
                  });
    waitingThreads = waitingThreads - 1;

    return ret;
}

#endif //_WIN32


}