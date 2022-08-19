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
    updateNodes.clear();
    renderNodes.clear();

    collectQueue.push(entryNode.get());

    while (!collectQueue.empty()) {
        auto *front = collectQueue.front();
        collectQueue.pop();

        updateNodes.push_back(front);

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

    renderNodes.push_back(fontManagerNode);
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

        ANAssert(renderQueue.init());
        ANAssert(audioEngine.init());

        GetRenderQueue().enqueue([] {
            ANAssert(GetRenderer().init());
            ANAssert(GetFontManager().init());
        });

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

        if (!entryNode->init()) {
            throw Exception("Fail to init entry node");
        }

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
            }

            if (needsRecollectNodes) {
                needsRecollectNodes = false;
                recollectNodes();

                /// update nodes to render
                GetRenderQueue().enqueue([renderNodes = renderNodes] {
                    GetRenderer().changeNodes(renderNodes);
                });
            }

            /// submit render
            GetRenderQueue().enqueue([deltaTime = timer.deltaTime, elapsedTime = timer.elapsedTime] {
                GetRenderer().render(deltaTime, elapsedTime);
            });



        }
#ifdef _WIN32
        /// restore the scheduler granularity
        timeEndPeriod(1);
#endif
        GetRenderQueue().enqueue([] {
            GetRenderer().changeNodes({});
            GetRenderer().willDeinit();
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