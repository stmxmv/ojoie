//
// Created by Aleudillonam on 7/30/2022.
//

#ifndef OJOIE_EVENT_HPP
#define OJOIE_EVENT_HPP

#include <ojoie/Export/Export.h>

#include <memory>
#include <condition_variable>

namespace AN {

/// \note multi threads waitting on one event need to be manualReset-ed
class AN_API Event {
public:

    Event();

    ~Event();

    bool init(bool bIsManualReset = false);

    bool isManualReset() const { return ManualReset; }

    void trigger();

    void reset();

    bool wait(int WaitTimeMs);

private:
#ifdef _WIN32
    struct Pimpl;
    Pimpl *pimpl;
#else
    std::mutex mutex;
    std::condition_variable cond;

    enum class TriggerType {
        none, all, one
    };

    volatile TriggerType triggered;
    volatile int waitingThreads;
    bool inited;
#endif
    /** Whether the signaled state of the event needs to be reset manually. */
    bool ManualReset;
};

}

#endif//OJOIE_EVENT_HPP
