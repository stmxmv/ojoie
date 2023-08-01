//
// Created by aojoie on 5/17/2023.
//

#include "Render/RenderManager.hpp"
#include "Render/TextureManager.hpp"
#include "Camera/Camera.hpp"
#include "Render/RenderLoop/ForwardRenderLoop.hpp"
#include "Render/LayerManager.hpp"
#include "Render/QualitySettings.hpp"
#include "Misc/ResourceManager.hpp"
#include "IMGUI/IMGUIManager.hpp"
#include "Render/Light.hpp"
#include "Modules/Dylib.hpp"

#include "Core/Actor.hpp"

#ifdef OJOIE_WITH_EDITOR
#include "RenderDoc/renderdoc_app.h"
#include "Editor/Selection.hpp"
#endif//OJOIE_WITH_EDITOR

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif//_WIN32

namespace AN {

#ifdef OJOIE_WITH_EDITOR
typedef RENDERDOC_API_1_0_0 RenderDocApi;
#define RenderDocApiVersion eRENDERDOC_API_Version_1_0_0

std::string gRenderDocAppPath;

RenderDocApi *gRenderDocApi = nullptr;

RenderDocApi *GetRenderDocApi() {
    return gRenderDocApi;
}

void LoadRenderDoc() {
    void *library = nullptr;
#ifdef _WIN32
    HKEY hKey;
    LPCWSTR subKey = L"SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\";
    LPCWSTR valueName = L"";
    WCHAR valueData[MAX_PATH];
    DWORD dataSize = MAX_PATH;

    std::filesystem::path renderDocDllPath;
    // Open the registry key
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Read the string value
        if (RegGetValue(hKey, nullptr, valueName, RRF_RT_REG_SZ, nullptr, (PVOID)&valueData, &dataSize) == ERROR_SUCCESS) {
            renderDocDllPath = valueData;
            gRenderDocAppPath = renderDocDllPath.string();
            renderDocDllPath.remove_filename();
            renderDocDllPath.append("renderdoc.dll");
        }
        // Close the registry key
        RegCloseKey(hKey);
    }

    if (!renderDocDllPath.empty()) {
        library = LoadDynamicLibrary(renderDocDllPath.string().c_str());
    }

#endif//_WIN32

    if (library) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)LookupSymbol(library, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI) {
            RenderDocApi *rdoc_api;
            int ret = RENDERDOC_GetAPI(RenderDocApiVersion, (void **)&rdoc_api);
            if (ret) {
                rdoc_api->SetFocusToggleKeys(nullptr, 0);
                rdoc_api->SetCaptureKeys(nullptr, 0);
                rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
                rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 1);
                rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 1);
                rdoc_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);

                AN_LOG(Info, "%s", "Loaded RenderDoc");

                gRenderDocApi = rdoc_api;
            }
        }
    }
}

AN_API void RenderDocStartCapture() {
    RenderDocApi *renderDocApi = GetRenderDocApi();
    if (renderDocApi && !renderDocApi->IsFrameCapturing()) {
        renderDocApi->StartFrameCapture(nullptr, nullptr);

        if (!renderDocApi->IsFrameCapturing()) {
            AN_LOG(Warning, "%s", "RenderDoc cannot start capture");
        }

        if (!renderDocApi->IsRemoteAccessConnected()) {
            UInt32 PID = renderDocApi->LaunchReplayUI(true, nullptr);

            if (0 == PID) {
                AN_LOG(Warning, "%s", "Fail to launch RenderDoc");
            }
        }
    }
}

AN_API void RenderDocEndCapture() {
    RenderDocApi *renderDocApi = GetRenderDocApi();
    if (renderDocApi) {
        renderDocApi->EndFrameCapture(nullptr, nullptr);
    }
}

static void DrawRendererAndChildren(Renderer *renderer, RenderContext context, const char *pass) {
    if (renderer == nullptr) return;
    renderer->render(context, pass);
    TransformComponent *transform = renderer->getTransform();
    for (const auto &child : transform->getChildren()) {
        Renderer *childRenderer = child->getComponent<Renderer>();
        DrawRendererAndChildren(childRenderer, context, pass);
    }
}
#endif//OJOIE_WITH_EDITOR

RenderManager::RenderManager()
#ifdef OJOIE_WITH_EDITOR
    : bCaptureNextFrame()
#endif//OJOIE_WITH_EDITOR
{}

bool RenderManager::init() {
    if (!GetIMGUIManager().init()) return false;
    while (!initializeTask.empty()) {
        initializeTask.front().run();
        initializeTask.pop();
    }

    _commandPool = CommandPool::newCommandPool();

    renderArea = GetQualitySettings().getCurrent().targetResolution;
    UInt32 samples = GetQualitySettings().getCurrent().antiAliasing;

    if (samples > 1) {
        msaaSamples  = samples;
        bMSAAEnabled = true;
    } else {
        bMSAAEnabled = false;
        msaaSamples  = 1;
    }

    /// create ui overlay render pass
    SubpassInfo   subpass_infos[1] = {};
    LoadStoreInfo load_store[1]    = {};

    // render target
    load_store[0].loadOp  = kAttachmentLoadOpClear;
    load_store[0].storeOp = kAttachmentStoreOpStore;

    subpass_infos[0].colorAttachments       = { 0 };

    RenderPassDescriptor renderPassDescriptor{};

    renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
    renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));

    RenderTargetDescriptor targetAttachment{};
    targetAttachment.format  = kRTFormatDefault;
    targetAttachment.width = 0; // width and height is ignored in d3d11 renderPass creation
    targetAttachment.height = 0;
    targetAttachment.samples = 1;

    if (!uiOverlayRenderPass.init(renderPassDescriptor)) return false;

    targetAttachment.format  = kRTFormatDefault;
    targetAttachment.width = renderArea.width;
    targetAttachment.height = renderArea.height;
    targetAttachment.samples = msaaSamples;

    screenRenderTarget = MakeObjectPtr<RenderTarget>();

    if (!screenRenderTarget->init(targetAttachment)) {
        return false;
    }

    if (bMSAAEnabled) {
        RenderTargetDescriptor resolveAttachment{};
        resolveAttachment.format  = kRTFormatDefault;
        resolveAttachment.width = renderArea.width;
        resolveAttachment.height = renderArea.height;
        resolveAttachment.samples = 1;

        resolveTexture = MakeObjectPtr<RenderTarget>();

        if (!resolveTexture->init(resolveAttachment)) {
            return false;
        }
    }

#ifdef OJOIE_WITH_EDITOR
    targetAttachment.samples = 1;
    SamplerDescriptor samplerDescriptor = Texture::DefaultSamplerDescriptor();
    samplerDescriptor.addressModeU = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeV = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeW = kSamplerAddressModeClampToEdge;
    sceneViewSelectedTarget = MakeObjectPtr<RenderTarget>();
    if (!sceneViewSelectedTarget->init(targetAttachment, samplerDescriptor)) {
        return false;
    }

    sceneViewSelectedTarget1 = NewObject<RenderTarget>();
    if (!sceneViewSelectedTarget1->init(targetAttachment, samplerDescriptor)) {
        return false;
    }
#endif//OJOIE_WITH_EDITOR
    return true;
}

void RenderManager::deinit() {

#ifdef OJOIE_WITH_EDITOR
    sceneViewSelectedTarget.reset();
#endif//OJOIE_WITH_EDITOR

    delete _commandPool;
    resolveTexture.reset();
    screenRenderTarget.reset();
    uiOverlayRenderPass.deinit();
    uiOverlayTarget.reset();
    GetIMGUIManager().deinit();
    while (!cleanupTasks.empty()) {
        cleanupTasks.front().run();
        cleanupTasks.pop();
    }
}



void RenderManager::performUpdate(UInt32 frameVersion) {
    UInt32 updateFrameIndex = frameVersion % kMaxFrameInFlight;

    GetLightManager().update();

    GetTextureManager().update(frameVersion);
    GetUniformBuffers().update();
    GetCameraManager().updateCameras(updateFrameIndex);
    updateRenderers(updateFrameIndex);

    /// update IMGUI
    GetIMGUIManager().onNewFrame();
    GetIMGUIManager().updateIMGUIComponents();
    GetIMGUIManager().update(frameVersion);
}

void RenderManager::performRender(TaskInterface completionHandler) {
#ifdef OJOIE_WITH_EDITOR
    if (bCaptureNextFrame) {
        RenderDocStartCapture();
        CommandBuffer::SetEmitDebugLabel(true);
    }
#endif//OJOIE_WITH_EDITOR

    static Material *gGUIBlitMat;
    if (gGUIBlitMat == nullptr) {
        Shader* s_BlitShader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "GUIBlit");
        ANAssert(s_BlitShader);
        gGUIBlitMat = NewObject<Material>();
        ANAssert(gGUIBlitMat->init(s_BlitShader, "GUIBlitMaterial"));
    }


    /// render to layer 0
    Layer *layer = GetLayerManager().getLayerAt(0);
    Presentable *presentable = layer->nextPresentable();
    if (!presentable) {
        /// skip this frame
        if (completionHandler) {
            completionHandler.run();
        }
        ++renderFrameVersion;
        return;
    }

    UInt32 frameIndex = presentable->getFrameIndex();
    _commandPool->reset();
    CommandBuffer *commandBuffer = _commandPool->newCommandBuffer();

    //// render pass code
    /// TODO multiple camera support
    float dpiScaleX, dpiScaleY;
    layer->getDpiScale(dpiScaleX, dpiScaleY);

    Size layerSize = layer->getSize();
    RenderContext renderContext;
    renderContext.frameWidth = renderArea.width;
    renderContext.frameHeight = renderArea.height;
    renderContext.layerWidth = layerSize.width;
    renderContext.layerHeight = layerSize.height;
    renderContext.dpiScaleX     = dpiScaleX;
    renderContext.dpiScaleY     = dpiScaleY;
    renderContext.frameVersion  = renderFrameVersion;
    renderContext.frameIndex    = renderFrameVersion % kMaxFrameInFlight;
    renderContext.commandBuffer = commandBuffer;

    /// render all cameras
    for (CameraListNode &node : GetCameraManager().getCameras()) {
        Camera &camera = *node;
        if (camera.getRenderTarget() == nullptr) {
            /// if camera's renderTarget is null, set it render to screen
            camera.setRenderTarget(screenRenderTarget.get());
        }
        commandBuffer->debugLabelBegin(std::format("Render Camera-{}-{}", camera.getActor().getName().c_str(), (void *)&camera).c_str(), { 0.7f, 0.4f, 0.1f, 1.f });
        camera.drawRenderers(renderContext, _rendererList);
        commandBuffer->debugLabelEnd(); // Render Camera
    }

#ifdef OJOIE_WITH_EDITOR

    /// render scene selected outline
    Actor *actor = Editor::Selection::GetActiveActor();
    if (actor) {
        Renderer *renderer = actor->getComponent<Renderer>();
        if (renderer) {
            Shader *shader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "SceneViewSelected");
            static Material *s_SceneViewSelectedMat = nullptr;
            if (s_SceneViewSelectedMat == nullptr) {
                s_SceneViewSelectedMat = NewObject<Material>();
                s_SceneViewSelectedMat->init(shader, "SceneViewSelectedMat");
            }
            commandBuffer->debugLabelBegin("Render Scene Selected", { 0.7f, 0.4f, 0.1f, 1.f });
            for (CameraListNode &node : GetCameraManager().getCameras()) {
                Material::SetReplacementShader(shader);
                node->beginRender();

                AttachmentDescriptor attachments[1] = {};
                attachments[0].format = kRTFormatDefault;
                attachments[0].loadStoreTarget = sceneViewSelectedTarget.get();
                attachments[0].loadOp = kAttachmentLoadOpClear;
                attachments[0].storeOp = kAttachmentStoreOpStore;
                attachments[0].clearColor = { 0.f, 0.f, 0.f, 0.f };

                commandBuffer->beginRenderPass(renderContext.frameWidth, renderContext.frameHeight, 1,
                                               attachments, -1);
                commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f, .width = (float) renderContext.frameWidth, .height = (float)renderContext.frameHeight, .znear = 0.f, .zfar = 1.f });
                commandBuffer->setScissor({ .x = 0, .y = 0, .width = (int)renderContext.frameWidth, .height = (int)renderContext.frameHeight });
                DrawRendererAndChildren(renderer, renderContext, "Forward");
                commandBuffer->endRenderPass();

                Material::SetReplacementShader(nullptr);

                Size size = node->getRenderTarget()->getSize();
                s_SceneViewSelectedMat->setVector("_TargetTexelSize", { 1.f / (float)size.width, 1.f / (float)size.height, (float)size.width, (float)size.height });

                commandBuffer->clearRenderTarget(sceneViewSelectedTarget1, { 0.f, 0.f, 0.f, 0.f });
                s_SceneViewSelectedMat->setVector("_BlurDirection", { 1.0, 0.f, 0.f, 0.f });
                commandBuffer->blitTexture(sceneViewSelectedTarget.get(), sceneViewSelectedTarget1, s_SceneViewSelectedMat, 1);

                commandBuffer->clearRenderTarget(sceneViewSelectedTarget.get(), { 0.f, 0.f, 0.f, 0.f });
                s_SceneViewSelectedMat->setVector("_BlurDirection", { 0.0, 1.f, 0.f, 0.f });
                commandBuffer->blitTexture(sceneViewSelectedTarget1, sceneViewSelectedTarget.get(), s_SceneViewSelectedMat, 1);
                commandBuffer->blitTexture(sceneViewSelectedTarget.get(), node->getRenderTarget(), s_SceneViewSelectedMat, 2);
            }
            commandBuffer->debugLabelEnd();
        }
    }

#endif//OJOIE_WITH_EDITOR

    /// render UI Overlay
    commandBuffer->debugLabelBegin("Render UI Overlay", { 0.7f, 0.4f, 0.1f, 1.f });
    if (!uiOverlayTarget) {
        recreateUIOverlayTarget(layer->getSize());
    }
    Size targetSize = uiOverlayTarget->getSize();
    renderContext.frameWidth    = targetSize.width;
    renderContext.frameHeight   = targetSize.height;

    const RenderTarget *renderTargets[] = { uiOverlayTarget.get() };
    ClearValue clearValue[1];
    clearValue[0].color = { { 0.f, 0.f, 0.f, 0.0f } };
    commandBuffer->beginRenderPass(renderContext.frameWidth, renderContext.frameHeight,
                                   uiOverlayRenderPass,
                                   renderTargets,
                                   clearValue);

    commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f,
                                 .width = (float) renderContext.frameWidth,
                                 .height = (float) renderContext.frameHeight, .znear = 0.f, .zfar = 1.f });
    /// do render IMGUI overlay
    GetIMGUIManager().render(renderContext);
    commandBuffer->endRenderPass();
    commandBuffer->debugLabelEnd(); // UI Overlay


    commandBuffer->debugLabelBegin("Blit screenRenderTarget", { 0.7f, 0.4f, 0.1f, 1.f });
    if (bMSAAEnabled) {
        commandBuffer->resolveTexture(screenRenderTarget.get(), 0, resolveTexture.get(), 0, kPixelFormatRGBA8Unorm);
        commandBuffer->blitTexture(resolveTexture.get(), presentable->getRenderTarget());
    } else {
        commandBuffer->blitTexture(screenRenderTarget.get(), presentable->getRenderTarget());
    }
    commandBuffer->debugLabelEnd(); //Blit screenRenderTarget

    commandBuffer->debugLabelBegin("Blit UI Overlay", { 0.7f, 0.4f, 0.1f, 1.f });
    commandBuffer->blitTexture(uiOverlayTarget.get(), presentable->getRenderTarget(), gGUIBlitMat);
    commandBuffer->debugLabelEnd(); //Blit UI Overlay

    /// register to present after submit
    commandBuffer->present(*presentable);

    commandBuffer->submit();

    if (completionHandler) {
        completionHandler.run();
    }

    ++renderFrameVersion;

#ifdef OJOIE_WITH_EDITOR
    if (bCaptureNextFrame) {
        bCaptureNextFrame = false;
        RenderDocEndCapture();
        CommandBuffer::SetEmitDebugLabel(false);
    }
#endif//OJOIE_WITH_EDITOR
}

void RenderManager::addRenderer(RendererListNode &renderer) {
    _rendererList.push_back(renderer);
}

void RenderManager::removeRenderer(RendererListNode &renderer) {
    renderer.removeFromList();
}


void RenderManager::updateRenderers(UInt32 frameIndex) {
    for (auto &node : _rendererList) {
        Renderer &renderer = *node;
        renderer.update(frameIndex);
    }
}

void RenderManager::recreateUIOverlayTarget(const Size &size) {
    RenderTargetDescriptor targetAttachment{};
    targetAttachment.format  = kRTFormatDefault;
    targetAttachment.width = size.width;
    targetAttachment.height = size.height;
    targetAttachment.samples = 1;

    uiOverlayTarget.reset();
    uiOverlayTarget = MakeObjectPtr<RenderTarget>();

    ANAssert(uiOverlayTarget->init(targetAttachment));
}

void RenderManager::onLayerSizeChange(int layerIndex, const Size &size) {
    if (layerIndex == 0) {
        Size viewportSize = size;
        if (viewportSize.width != 0 && viewportSize.height != 0) {
            float ratio = (float) viewportSize.width / (float) viewportSize.height;
            for (CameraListNode &node : GetCameraManager().getCameras()) {
                if (node->isMatchLayerRatio()) {
                    node->setViewportRatio(ratio);
                }
            }
            recreateUIOverlayTarget(viewportSize);
        }
    }
}

RenderManager &GetRenderManager() {
    static RenderManager RenderManager;
    return RenderManager;
}


}