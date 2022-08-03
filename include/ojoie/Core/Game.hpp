//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_GAME_HPP
#define OJOIE_GAME_HPP

#include <ojoie/Core/typedef.h>
#include <ojoie/Core/Node.hpp>
#include <ojoie/Core/Timer.hpp>
#include <ojoie/Core/delegate.hpp>
#include <ojoie/Core/Task.hpp>
#include <thread>
#include <semaphore>
#include <queue>

namespace AN {


class Game : private NonCopyable {
    struct Impl;
    Impl *impl;

    std::atomic_bool isStop;

    int _maxFrameRate;

    std::counting_semaphore<> renderSemaphore;

    std::thread gameThread;

    Timer timer;

    std::queue<Node *> collectQueue;
    std::vector<Node *> updateNodes;
    std::vector<std::shared_ptr<Node>> renderNodes;

    std::vector<TaskInterface> cleanupTasks;

    Game();
    ~Game();
    void recollectNodes();
public:

    static Game &GetSharedGame();

    bool init();

    void start();

    void deinit();

    template<typename Func>
    void registerCleanupTask(Func &&func) {
        cleanupTasks.push_back(TaskItem(std::forward<Func>(func)));
    }

    bool needsRecollectNodes;

    std::shared_ptr<Node> entryNode;

    std::atomic<float> deltaTime;
    std::atomic<float> elapsedTime;

    void setMaxFrameRate(int rate) {
        _maxFrameRate = rate;
    }

    constexpr static int MaxFramesInFlight = 3;
};


inline Game &GetGame() {
    return Game::GetSharedGame();
}


}

#endif//OJOIE_GAME_HPP
