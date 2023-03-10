//
// Created by Aleudillonam on 7/27/2022.
//

#include "Core/Game.hpp"
#include "Core/Dispatch.hpp"
#include "Core/Log.h"

#include "Audio/Sound.hpp"

#include "Render/Renderer.hpp"
#include "Render/RenderQueue.hpp"
#include "Render/Font.hpp"

#include "Core/Configuration.hpp"
#include "Input/InputManager.hpp"

#include <concurrentqueue/concurrentqueue.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#pragma comment(lib, "Winmm.lib")
#endif



namespace AN {

struct Game::Impl {
    moodycamel::ConcurrentQueue<TaskInterface> dispatchTasks;

};

Game::Game() : _maxFrameRate(INT_MAX), renderSemaphore(MaxFramesInFlight), needsRecollectNodes(), impl(new Impl()) {
    Dispatch::GetDelegate()[Dispatch::Game] = [] (const TaskInterface &task) {
        GetGame().impl->dispatchTasks.enqueue(task);
    };

    GetConfiguration().setObject("forward-shading", false);
    GetConfiguration().setObject("anti-aliasing", "TAA");
}

Game::~Game() {
    delete impl;
}

Game &Game::GetSharedGame() {
    static Game game;
    return game;
}


bool Game::init() {

    return true;
}

void Game::deinit() {
    isStop = true;
    gameThread.join();
}

void Game::recollectNodes() {
    enum class RenderNodeState {
        Old, Common, New, Removed
    };

    std::unordered_map<Node *, RenderNodeState> nodeRenderStateMap;

    for (auto &node : renderNodes) {
        nodeRenderStateMap[node.get()] = RenderNodeState::Old;
    }

    updateNodes.clear();
    renderNodes.clear();

    collectQueue.push(entryNode.get());

    while (!collectQueue.empty()) {
        auto *front = collectQueue.front();
        collectQueue.pop();

        updateNodes.push_back(front);

        if (nodeRenderStateMap.contains(front)) {
            nodeRenderStateMap[front] = RenderNodeState::Common;
        } else {
            nodeRenderStateMap[front] = RenderNodeState::New;
        }

        renderNodes.push_back(front->shared_from_this());

        for (auto &child : front->_children) {
            collectQueue.push(child.get());
        }
    }

    static std::shared_ptr<FontManagerNode> fontManagerNode = [] {
        auto fontManagerNode = FontManagerNode::Alloc();
        ANAssert(fontManagerNode->init());
        return fontManagerNode;
    }();


    if (nodeRenderStateMap.contains(fontManagerNode.get())) {
        nodeRenderStateMap[fontManagerNode.get()] = RenderNodeState::Common;
    } else {
        nodeRenderStateMap[fontManagerNode.get()] = RenderNodeState::New;
    }

    renderNodes.push_back(fontManagerNode);


    for (auto &&[node, state] : nodeRenderStateMap) {
        if (state == RenderNodeState::New) {
            newRenderNodes.push_back(node);
        } else if (state == RenderNodeState::Old) {
            removedRenderNodes.push_back(node);
        }

    }

    needsRecollectNodes = false;
}


void Game::start() {
    if (!entryNode) {
        throw Exception("Game has no entry!");
    }
    gameThread = std::thread([this] {

        std::atomic_thread_fence(std::memory_order_seq_cst);

        Dispatch::SetThreadID(Dispatch::Game, std::this_thread::get_id());
#ifdef _WIN32
        SetThreadDescription(GetCurrentThread(),L"com.an.GameThread");
#endif

        ANLog("Game start");

        moodycamel::ConsumerToken token(impl->dispatchTasks);

        GetRenderer().completionHandler = [this] {
            renderSemaphore.release();
        };

        RenderQueue &renderQueue = GetRenderQueue();
        AudioEngine &audioEngine = GetAudioEngine();

#define CHECK_INIT(statement)                    \
    do {                                         \
        if (!statement) {                         \
            Dispatch::async(Dispatch::Main, [] { \
                throw Exception(#statement);     \
            });                                  \
            return;                              \
        }                                        \
    } while (0)



        CHECK_INIT(renderQueue.init());
        CHECK_INIT(audioEngine.init());

        /// render global stuff inited in render thread
        bool fail = false;
        GetRenderQueue().enqueue([&fail] {
            fail = true;
            CHECK_INIT(GetRenderer().init());
            CHECK_INIT(GetFontManager().init());
            fail = false;
        });

        RenderFence fence0;
        fence0.wait();

        if (fail) {
            return;
        }

        GetRenderQueue().registerCleanupTask([] {
            GetFontManager().deinit();
            GetRenderer().deinit();
        });

        registerCleanupTask([]{
            GetAudioEngine().deinit();
            GetRenderQueue().stopAndWait();
        });



        /// run init task if any
        {
            TaskInterface task;
            while (impl->dispatchTasks.try_dequeue(token, task)) {
                task.run();
            }
        }

        RenderFence fence;
        fence.wait();

        CHECK_INIT(entryNode->init());

        recollectNodes();

#ifdef _WIN32
        /// set the scheduler granularity to 1 ms
        timeBeginPeriod(1);
#endif
        while (!isStop) {

            TaskInterface task;
            while (impl->dispatchTasks.try_dequeue(token, task)) {
                task.run();
            }

            if (_maxFrameRate != INT_MAX) {
                float delta = timer.peek();
                float expect_delta = 1.f / (float)_maxFrameRate;
                if (delta < expect_delta) {
                    delta = expect_delta - delta;
                    if (delta > 0.005f) {
                        std::this_thread::sleep_for(std::chrono::milliseconds((int)(delta * 1000) - 2));
                    } else {
                        std::this_thread::yield();
                    }
                    cpu_relax();
                    cpu_relax();
                    continue;
                }
            }

            if (!renderSemaphore.try_acquire_for(std::chrono::seconds(3))) {
                continue;
            }


            timer.mark();
            deltaTime = timer.deltaTime;
            elapsedTime = timer.elapsedTime;

            GetInputManager().process(timer.deltaTime);

            for (auto *node : updateNodes) {
                if (node->tick) {
                    node->update(timer.deltaTime);
                }

                if (node->sceneProxy) {
                    node->updateSceneProxy();
                }

            }

            if (needsRecollectNodes) {

                recollectNodes();

            }

            for (auto &node : newRenderNodes) {
                renderScene.addNode(node->shared_from_this());
            }

            newRenderNodes.clear();

            for (auto &node : removedRenderNodes) {
                renderScene.removeNode(node->shared_from_this());
            }

            removedRenderNodes.clear();

            /// submit render
            GetRenderQueue().enqueue([deltaTime = timer.deltaTime, elapsedTime = timer.elapsedTime, this] {
                renderScene.updateSceneProxies();
                GetRenderer().render(renderScene, deltaTime, elapsedTime);
            });



        }
#ifdef _WIN32
        /// restore the scheduler granularity
        timeEndPeriod(1);
#endif
        GetRenderQueue().enqueue([this] {
            GetRenderer().willDeinit();

            renderScene.deinit();
        });

        /// make sure renderer release all nodes and proxies
        RenderFence renderFence;
        renderFence.wait();

        entryNode = nullptr;
        renderNodes.clear();
        updateNodes.clear();

        while (!cleanupTasks.empty()) {
            cleanupTasks.top().run();
            cleanupTasks.pop();
        }

    });
}


}