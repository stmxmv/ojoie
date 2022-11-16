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
#include <ojoie/Core/SpinLock.hpp>
#include <ojoie/Render/Scene.hpp>
#include <thread>
#include <semaphore>
#include <queue>
#include <stack>
#include <mutex>

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

    std::vector<Node *> newRenderNodes;
    std::vector<Node *> removedRenderNodes;

    RC::Scene renderScene;

    std::stack<TaskInterface> cleanupTasks;

    SpinLock selfLock;

    Game();
    ~Game();
    void recollectNodes();
public:

    static Game &GetSharedGame();

    bool init();

    void start();

    void deinit();

    /// \AnyActor
    template<typename Func>
    void registerCleanupTask(Func &&func) {
        std::lock_guard lock(selfLock);
        cleanupTasks.push(TaskItem(std::forward<Func>(func)));
    }

    bool needsRecollectNodes;

    float width, height;// in pixels

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
