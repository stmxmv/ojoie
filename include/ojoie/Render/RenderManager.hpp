//
// Created by aojoie on 5/17/2023.
//

#ifndef OJOIE_RENDERERMANAGER_HPP
#define OJOIE_RENDERERMANAGER_HPP

#include <ojoie/Render/Renderer.hpp>
#include <ojoie/Render/RenderLoop/RenderLoop.hpp>
#include <ojoie/Render/CommandPool.hpp>
#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Threads/Task.hpp>
#include <ojoie/Object/ObjectPtr.hpp>
#include <queue>


namespace AN {

class AN_API RenderManager {

#ifdef OJOIE_WITH_EDITOR
    bool bCaptureNextFrame;
    ObjectPtr<RenderTarget> sceneViewSelectedTarget;
    RenderTarget *sceneViewSelectedTarget1;
#endif//OJOIE_WITH_EDITOR

    RendererList _rendererList;

    CommandPool * _commandPool;

    UInt32 renderFrameVersion;

    std::queue<TaskInterface> initializeTask, cleanupTasks;

    bool bMSAAEnabled;
    UInt32 msaaSamples;

    Size renderArea{};

    ObjectPtr<RenderTarget> screenRenderTarget;
    ObjectPtr<RenderTarget> resolveTexture;
    ObjectPtr<RenderTarget> uiOverlayTarget;
    RenderPass uiOverlayRenderPass;

    void updateRenderers(UInt32 frameIndex);

    void recreateUIOverlayTarget(const Size &size);

public:

    RenderManager();

    bool init();

    void deinit();

    void registerInitializeTask(TaskInterface task) {
        initializeTask.push(std::move(task));
    }
    void registerCleanupTask(TaskInterface task) {
        cleanupTasks.push(std::move(task));
    }

    template<typename _Func>
    void registerInitializeTask(_Func &&task) {
        TaskItem taskItem{ task };
        registerInitializeTask(TaskInterface{ std::move(taskItem) });
    }
    template<typename _Func>
    void registerCleanupTask(_Func &&task) {
        TaskItem taskItem{ task };
        registerCleanupTask(TaskInterface{ std::move(taskItem) });
    }

#ifdef OJOIE_WITH_EDITOR
    void captureNextFrame() { bCaptureNextFrame = true; }
#endif//OJOIE_WITH_EDITOR

    /// in game thread
    void performUpdate(UInt32 frameVersion);

    void performRender(TaskInterface completionHandler);

    void addRenderer(RendererListNode &renderer);
    void removeRenderer(RendererListNode &renderer);

    const RendererList &getRendererList() const { return _rendererList; }

    /// delegate
    void onLayerSizeChange(int layerIndex, const Size &size);
};

AN_API RenderManager &GetRenderManager();

}
#endif//OJOIE_RENDERERMANAGER_HPP
