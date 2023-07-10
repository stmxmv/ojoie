//
// Created by aojoie on 4/23/2023.
//

#include "Render/RenderLoop/ForwardRenderLoop.hpp"
#include "Render/RenderTypes.hpp"

#include "Core/Window.hpp"
#include "Render/QualitySettings.hpp"

#include <concurrentqueue/blockingconcurrentqueue.hpp>

#include <Camera/Camera.hpp>
#include <Render/Renderer.hpp>

#include <thread>

namespace AN {

ForwardRenderLoop::ForwardRenderLoop() : renderTarget() {}

ForwardRenderLoop::~ForwardRenderLoop() {
    deinit();
}

void ForwardRenderLoop::deinit() {
    depthTexture.reset();
    renderPass.deinit();
}

bool ForwardRenderLoop::createRenderPass() {
    /// init render pass descriptor
    SubpassInfo   subpass_infos[1] = {};
    LoadStoreInfo load_store[2]    = {};

    // render target
    load_store[0].loadOp  = kAttachmentLoadOpClear;
    load_store[0].storeOp = kAttachmentStoreOpStore;

    // Depth
    load_store[1].loadOp  = kAttachmentLoadOpClear;
    load_store[1].storeOp = kAttachmentStoreOpDontCare;

    subpass_infos[0].colorAttachments       = { 0 };
    subpass_infos[0].depthStencilAttachment = 1;


    RenderPassDescriptor renderPassDescriptor{};

    renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
    renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));

    AttachmentDescriptor targetAttachment{};
    targetAttachment.format  = kRTFormatDefault;
    targetAttachment.width = 0;
    targetAttachment.height = 0;
    targetAttachment.samples = 1;


    AttachmentDescriptor depthAttachment{};
    depthAttachment.samples = 1;
    depthAttachment.format  = kRTFormatDepth;
    depthAttachment.width   = 0;
    depthAttachment.height  = 0;

    renderPassDescriptor.attachments.push_back(targetAttachment);
    renderPassDescriptor.attachments.push_back(depthAttachment);

    if (!renderPass.init(renderPassDescriptor)) {
        return false;
    }

    return true;
}

void ForwardRenderLoop::recreateAttachments(const Size &size, UInt32 msaaSamples) {
    depthTexture.reset();

    AttachmentDescriptor depthAttachment{};
    depthAttachment.samples = msaaSamples;
    depthAttachment.format  = kRTFormatDepth;
    depthAttachment.width   = size.width;
    depthAttachment.height  = size.height;
    depthTexture = MakeObjectPtr<RenderTarget>();

    ANAssert(depthTexture->init(depthAttachment));
}

bool ForwardRenderLoop::init() {

    if (!createRenderPass()) {
        return false;
    }

    return true;
}

void ForwardRenderLoop::setRenderTarget(RenderTarget *target) {
    renderTarget = target;
    Size renderArea = target->getSize();
    UInt32 massSamples = target->getMSAASamples();

    if (_renderArea != renderArea || _msaaSamples != massSamples || depthTexture == nullptr) {
        recreateAttachments(renderArea, massSamples);

        _renderArea = renderArea;
        _msaaSamples = massSamples;
    }
}

void ForwardRenderLoop::performUpdate(UInt32 frameVersion) {

}

void ForwardRenderLoop::performRender(RenderContext &context,
                                      PerformRenderCallback renderCode, void *userdata) {

    context.renderPass = &renderPass;

    CommandBuffer *commandBuffer = context.commandBuffer;

    commandBuffer->debugLabelBegin("ForwardRenderLoop", { 0.7f, 0.4f, 0.1f, 1.f });

    ClearValue clearValue[2];
    clearValue[0].color        = { { 0.f, 0.f, 0.f, 1.0f } };
    clearValue[1].depthStencil = { 1.0f, 0 };

    const RenderTarget *renderTargets[] = { renderTarget, depthTexture.get() };
    commandBuffer->beginRenderPass(_renderArea.width, _renderArea.height,
                                   renderPass,
                                   renderTargets,
                                   clearValue);


    /// setting viewport and scissors
    commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f, .width = (float) _renderArea.width, .height = (float) _renderArea.height, .znear = 0.f, .zfar = 1.f });

    commandBuffer->setScissor({ .x = 0, .y = 0, .width = (int) _renderArea.width, .height = (int) _renderArea.height });

    /// real render here
    renderCode(context, userdata);

    commandBuffer->endRenderPass();
    commandBuffer->debugLabelEnd(); // ForwardRenderLoop
}

}// namespace AN