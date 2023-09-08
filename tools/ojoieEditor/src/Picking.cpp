//
// Created by Aleudillonam on 7/30/2023.
//

#include "Picking.hpp"

#include <ojoie/Render/CommandPool.hpp>
#include <ojoie/Misc/ResourceManager.hpp>
#include <ojoie/Render/RenderManager.hpp>
#include <ojoie/Editor/Selection.hpp>

namespace AN {

void RenderDocStartCapture();
void RenderDocEndCapture();

}

namespace AN::Editor {


static void EncodeIndex(UInt32 index, UInt8 color[4]) {
    // output is R,G,B,A bytes; index is ABGR dword
    color[0] = index & 0xFF;
    color[1] = (index >> 8) & 0xFF;
    color[2] = (index >> 16) & 0xFF;
    color[3] = (index >> 24) & 0xFF;
}

static UInt32 DecodeIndex(UInt32 argb) {
    // input is color from ARGB32 format pixel; index is ABGR dword
    // ARGB32 in memory on little endian looks like BGRA in dword
    return ((argb >> 8) & 0xFFFFFF) | ((argb & 0xFF) << 24);
}


void PickObject(Camera &camera, const Vector2f &pos) {

//    CommandBuffer::SetEmitDebugLabel(true);
//    RenderDocStartCapture();

    CommandBuffer *commandBuffer = GetCommandPool().newCommandBuffer();
    commandBuffer->debugLabelBegin("Editor Picking", { 0.7f, 0.4f, 0.1f, 1.f });

    static Shader *pickShader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "SceneViewSelection");
    static RenderTarget *renderTarget = nullptr;
    static RenderTarget *depthTarget = nullptr;

    UInt32 width = 1920;
    UInt32 height = 1080;

    RenderContext renderContext{};
    renderContext.frameWidth = 1920;
    renderContext.frameHeight = 1080;
    renderContext.dpiScaleX     = 1.f;
    renderContext.dpiScaleY     = 1.f;
    renderContext.commandBuffer = commandBuffer;

    if (renderTarget == nullptr) {
        renderTarget = NewObject<RenderTarget>();
        RenderTargetDescriptor attachmentDescriptor{};
        attachmentDescriptor.width = 1920;
        attachmentDescriptor.height = 1080; /// width and height according to viewport size
        attachmentDescriptor.samples = 1;
        attachmentDescriptor.format = kRTFormatDefault;

        renderTarget->init(attachmentDescriptor);

        depthTarget = NewObject<RenderTarget>();
        attachmentDescriptor.format = kRTFormatDepth;

        depthTarget->init(attachmentDescriptor);
    }

    Material::SetReplacementShader(pickShader);

    AttachmentDescriptor attachments[2] = {};
    attachments[0].format = kRTFormatDefault;
    attachments[0].loadStoreTarget = renderTarget;
    attachments[0].loadOp = kAttachmentLoadOpClear;
    attachments[0].storeOp = kAttachmentStoreOpStore;
    attachments[0].clearColor = { 0.f, 0.f, 0.f, 0.f };

    attachments[1].format = kRTFormatDefault;
    attachments[1].loadStoreTarget = depthTarget;
    attachments[1].loadOp = kAttachmentLoadOpClear;
    attachments[1].storeOp = kAttachmentStoreOpDontCare;

    Vector2f size{ 2.f, 2.f };
    int      minx  = RoundfToInt(pos.x - size.x / 2.0F);
    int      miny  = RoundfToInt(pos.y - size.y / 2.0F);
    int      maxx  = RoundfToInt(pos.x + size.x / 2.0F);
    int      maxy  = RoundfToInt(pos.y + size.y / 2.0F);
    int      xSize = maxx - minx;
    int      ySize = maxy - miny;

    commandBuffer->beginRenderPass(1920, 1080, 1, attachments, 1);
    commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f, .width = 1920.f, .height = 1080.f, .znear = 0.f, .zfar = 1.f });
    commandBuffer->setScissor({ .x = minx, .y = miny, .width = xSize, .height = ySize });

    camera.beginRender();

    std::vector<Renderer *> renderers;
    for (const auto &node : GetRenderManager().getRendererList()) {
        renderers.push_back(node.getData());
    }

    for (int i = 0; i < renderers.size(); ++i) {
        UInt8 encoded[4];
        EncodeIndex(i + 1, encoded);
        Material::SetVectorGlobal("_SelectionID", { ByteToNormalized(encoded[0]), ByteToNormalized(encoded[1]), ByteToNormalized(encoded[2]), ByteToNormalized(encoded[3]) });
        renderers[i]->render(renderContext, "Forward");
    }

    commandBuffer->endRenderPass();
    Material::SetReplacementShader(nullptr);
    commandBuffer->debugLabelEnd();


    UInt32 *imageData = (UInt32 *)malloc(xSize * ySize * 4);

    Object *pickObject = nullptr;
    std::vector<int> scores(renderers.size());
    int bestScore = -1;
    if (commandBuffer->readTexture(imageData, renderTarget->getTextureID(), minx, miny, xSize, ySize)) {
        for( int i = 0; i < xSize * ySize; ++i ) {
            UInt32 col = imageData[i];
            UInt32 index = DecodeIndex(col);
            --index;

            if (index < renderers.size()) {
                ++scores[index];
                if (scores[index] > bestScore) {
                    bestScore = scores[index];
                    pickObject = renderers[index];
                }
            }
        }
    }

    commandBuffer->submit();
    free(imageData);

    Selection::SetActiveObject(pickObject);

//    RenderDocEndCapture();
}

}