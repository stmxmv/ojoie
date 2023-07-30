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
}

bool ForwardRenderLoop::createRenderPass() {

    return true;
}

void ForwardRenderLoop::recreateAttachments(const Size &size, UInt32 msaaSamples) {
    depthTexture.reset();

    RenderTargetDescriptor depthAttachment{};
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

    CommandBuffer *commandBuffer = context.commandBuffer;

    commandBuffer->debugLabelBegin("ForwardRenderLoop", { 0.7f, 0.4f, 0.1f, 1.f });

    ClearValue clearValue[2];
    clearValue[0].color        = { { 0.f, 0.f, 0.f, 1.0f } };
    clearValue[1].depthStencil = { 1.0f, 0 };


    AttachmentDescriptor attachments[2] = {};

    attachments[0].format = kRTFormatDefault;
    attachments[0].loadStoreTarget = renderTarget;
    attachments[0].loadOp = kAttachmentLoadOpClear;
    attachments[0].storeOp = kAttachmentStoreOpStore;

    attachments[1].format = kRTFormatDepth;
    attachments[1].loadStoreTarget = depthTexture.get();
    attachments[1].loadOp = kAttachmentLoadOpClear;
    attachments[1].storeOp = kAttachmentStoreOpDontCare;

    commandBuffer->beginRenderPass(_renderArea.width, _renderArea.height, _msaaSamples,
                                   attachments, 1);


    /// setting viewport and scissors
    commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f, .width = (float) _renderArea.width, .height = (float) _renderArea.height, .znear = 0.f, .zfar = 1.f });

    commandBuffer->setScissor({ .x = 0, .y = 0, .width = (int) _renderArea.width, .height = (int) _renderArea.height });

    /// real render here
    renderCode(context, userdata);

    commandBuffer->endRenderPass();
    commandBuffer->debugLabelEnd(); // ForwardRenderLoop
}

}// namespace AN