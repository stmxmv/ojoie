//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_GAME_HPP
#define OJOIE_GAME_HPP

#include "ojoie/Configuration/typedef.h"

#include <ojoie/Object/ObjectPtr.hpp>
#include <ojoie/Core/Actor.hpp>
#include <ojoie/Template/delegate.hpp>
#include <ojoie/Threads/SpinLock.hpp>
#include <ojoie/Threads/Task.hpp>
#include <ojoie/Utility/Timer.hpp>

#include <mutex>
#include <queue>
#include <semaphore>
#include <stack>
#include <thread>
#include <list>
#include <span>

namespace AN {


class AN_API Game : private NonCopyable {
    struct Impl;
    Impl *impl;

    bool bIsStarted;
    int _maxFrameRate;

    UInt32 frameVersion;

    std::counting_semaphore<> renderSemaphore;

    Timer timer;

    std::stack<TaskInterface> cleanupTasks;

    SpinLock selfLock;

    Game();

    ~Game();

public:

    static Game &GetSharedGame();

    bool init();

    void start();

    void deinit();

    void tick();

    void performMainLoop();

    /// \AnyActor
    template<typename Func>
    void registerCleanupTask(Func &&func) {
        std::lock_guard lock(selfLock);
        cleanupTasks.push(TaskItem(std::forward<Func>(func)));
    }

    bool needsRecollectNodes;

    float width, height;// in pixels

    float deltaTime;
    float elapsedTime;

    void setMaxFrameRate(int rate) {
        _maxFrameRate = rate;
    }
};


inline Game &GetGame() {
    return Game::GetSharedGame();
}


}

#endif//OJOIE_GAME_HPP
