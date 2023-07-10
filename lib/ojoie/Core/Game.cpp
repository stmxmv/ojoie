//
// Created by Aleudillonam on 7/27/2022.
//

#include "Core/Game.hpp"
#include "Threads/Dispatch.hpp"
#include "Utility/Log.h"
#include "Template/Access.hpp"
#include "Core/Behavior.hpp"

#include "Audio/Sound.hpp"

#include "Render/RenderQueue.hpp"
#include "Render/RenderManager.hpp"

#include "Core/Configuration.hpp"
#include "Input/InputManager.hpp"
#include "Render/RenderContext.hpp"
#include "Misc/ResourceManager.hpp"

#include <concurrentqueue/concurrentqueue.hpp>

#ifdef _WIN32
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")
#undef RegisterClass

#endif



namespace AN {


struct Game::Impl {
};

Game::Game() : _maxFrameRate(INT_MAX), renderSemaphore(kMaxFrameInFlight), needsRecollectNodes(), impl(new Impl()) {
    Dispatch::GetDelegate()[Dispatch::Game] = Dispatch::GetDelegate()[Dispatch::Main];
    bIsStarted = false;
}

Game::~Game() {
    delete impl;
}

Game &Game::GetSharedGame() {
    static Game game;
    return game;
}


bool Game::init() {
    frameVersion = 0;
    return true;
}

void Game::deinit() {
    bIsStarted = false;
#ifdef _WIN32
    /// restore the scheduler granularity
    timeEndPeriod(1);
#endif

    RenderContextWaitIdle();

    while (!cleanupTasks.empty()) {
        cleanupTasks.top().run();
        cleanupTasks.pop();
    }

}

void Game::tick() {
//    renderSemaphore.acquire();

    timer.mark();
    deltaTime = timer.deltaTime;
    elapsedTime = timer.elapsedTime;

    GetInputManager().update();

    /// update behavior component
    GetBehaviorManager().update();


    GetRenderManager().performUpdate(frameVersion);

//    std::atomic_thread_fence(std::memory_order_acq_rel);

    /// perform render
    GetRenderManager().performRender(TaskItem([this] {
//        renderSemaphore.release();
    }));

    GetInputManager().onNextUpdate();

    ++frameVersion;
}

void Game::performMainLoop() {
    if (!bIsStarted) return;

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
            return;
        }
    }

    tick();
}

void Game::start() {

    Dispatch::SetThreadID(Dispatch::Game, GetCurrentThreadID());
    SetCurrentThreadName("com.an.GameThread");

    ANLog("Game start");

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


    GetResourceManager().loadBuiltinResources();

    CHECK_INIT(GetInputManager().init());
    CHECK_INIT(renderQueue.init());
    CHECK_INIT(audioEngine.init());
    CHECK_INIT(GetRenderManager().init());

    //    /// render global stuff inited in render thread
    //    bool fail = false;
    //    GetRenderQueue().enqueue([&fail] {
    //        fail = true;
    //        CHECK_INIT(GetFontManager().init());
    //        fail = false;
    //    });


    //    GetRenderQueue().registerCleanupTask([] {
    //        GetFontManager().deinit();
    //    });




    registerCleanupTask([&]{
        GetRenderManager().deinit();
        GetAudioEngine().deinit();
        GetRenderQueue().stopAndWait();
        GetInputManager().deinit();
    });

#ifdef _WIN32
    /// set the scheduler granularity to 1 ms
    timeBeginPeriod(1);
#endif

    bIsStarted = true;
}


}