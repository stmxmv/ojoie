//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_COMMANDBUFFER_HPP
#define OJOIE_COMMANDBUFFER_HPP

#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/RenderTypes.hpp>
namespace AN {

class RenderPass;
class Presentable;
class RenderPipelineState;
class Texture;
class DynamicVertexBuffer;
class Material;

typedef UInt64 TextureID;

enum PipelineStageFlagBits {
    kPipelineStageNone                  = 0,
    kPipelineStageVertexInput           = 1 << 0,
    kPipelineStageVertexShader          = 1 << 1,
    kPipelineStageTransfer              = 1 << 2,
    kPipelineStageColorAttachmentOutput = 1 << 3,
    kPipelineStageTopOfPipe             = 1 << 4,
    kPipelineStageEarlyFragmentTests    = 1 << 5,
    kPipelineStageLateFragmentTests     = 1 << 6,
    kPipelineStageBottomOfPipe          = 1 << 7
};

typedef ANFlags PipelineStageFlags;

enum AccessFlagBits {
    kAccessNone                        = 0,
    kAccessTransferRead                = 1 << 0,
    kAccessTransferWrite               = 1 << 1,
    kAccessShaderRead                  = 1 << 2,
    kAccessShaderWrite                 = 1 << 3,
    kAccessColorAttachmentWrite        = 1 << 4,
    kAccessDepthStencilAttachmentRead  = 1 << 5,
    kAccessDepthStencilAttachmentWrite = 1 << 6
};

typedef ANFlags AccessFlags;

enum TextureAspectFlagBits {
    kTextureAspectNone   = 0,
    kTextureAspectColor  = 1 << 0,
    kTextureAspectDepth  = 1 << 1,
    TextureAspectStencil = 1 << 2
};

typedef ANFlags TextureAspectFlags;

enum TextureLayout {
    kTextureLayoutColorAttachmentOptimal = 0,
    kTextureLayoutPresentSrc,
    kTextureLayoutUndefined,
    kTextureLayoutDepthStencilAttachmentOptimal,
    kTextureLayoutTransferSrcOptimal,
    kTextureLayoutTransferDstOptimal,
};

enum BlitFilter {
    kBlitFilterNearest = 0,
    kBlitFilterLinear
};

struct TextureBarrier {
    PipelineStageFlags srcStageFlag;
    PipelineStageFlags dstStageFlag;
    AccessFlags        srcAccessMask;
    AccessFlags        dstAccessMask;
    TextureAspectFlags aspectFlag;
    TextureLayout      oldLayout;
    TextureLayout      newLayout;
    UInt32             baseMipLevel;
    UInt32             levelCount;
    TextureID          textureID;
};


class AN_API CommandBuffer {
public:

    static void SetEmitDebugLabel(bool emit);
    static bool IsEmittingDebugLabel();

    virtual ~CommandBuffer() = default;

    virtual void submit() = 0;

    /// \brief wait single command buffer to complete
    virtual void waitUntilCompleted() = 0;

    virtual bool isCompleted() = 0;

    /// user can not call reset, unless reset mode is individual, other modes commandPool will manage
    virtual void reset() = 0;

    /// commands below

    virtual void debugLabelBegin(const char *name, Vector4f color) = 0;

    virtual void debugLabelEnd() = 0;

    virtual void debugLabelInsert(const char *name, Vector4f color) = 0;

    virtual void beginRenderPass(UInt32 width, UInt32 height,
                                 RenderPass                     &renderPass,
                                 std::span<const RenderTarget *> renderTargets,
                                 std::span<ClearValue>           clearValues) = 0;

    virtual void beginRenderPass(UInt32 width, UInt32 height, UInt32 samples,
                                 std::span<const AttachmentDescriptor> attachments,
                                 int depthAttachmentIndex) = 0;

    virtual void nextSubPass() = 0;

    virtual void endRenderPass() = 0;

    virtual void setViewport(const Viewport &viewport) = 0;

    virtual void setScissor(const ScissorRect &scissorRect) = 0;

    virtual void setCullMode(CullMode cullMode) = 0;

    virtual void setRenderPipelineState(AN::RenderPipelineState &renderPipelineState) = 0;

    /// note that some graphic api backend don't support this
    virtual void pushConstants(uint32_t offset, uint32_t size, const void *data) = 0;

    virtual void drawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;

    virtual void draw(uint32_t count) = 0;

    virtual void immediateVertex(float x, float y, float z) = 0;

    virtual void immediateNormal(float x, float y, float z) = 0;

    virtual void immediateColor(float r, float g, float b, float a) = 0;

    virtual void immediateTexCoordAll(float x, float y, float z) = 0;

    virtual void immediateTexCoord(int unit, float x, float y, float z) = 0;

    virtual void immediateBegin(PrimitiveType type) = 0;

    virtual void immediateEnd() = 0;

    virtual void textureBarrier(const TextureBarrier &textureBarrier) = 0;

    virtual void textureBarrier(std::span<const TextureBarrier> textureBarriers) = 0;

    virtual void blitTexture(TextureID srcTexID, Int32 srcWidth, Int32 srcHeight, UInt32 srcMipLevel,
                             TextureID dstTexID, Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                             TextureAspectFlags aspectMask,
                             BlitFilter         filter) = 0;

    virtual void blitTexture(Texture *tex, RenderTarget *target) = 0;

    virtual void blitTexture(Texture *tex, RenderTarget *target, Material *material, int pass = 0) = 0;

    virtual void clearRenderTarget(RenderTarget  *target, const Vector4f &color = { 0.f, 0.f, 0.f, 1.f }) = 0;

    virtual void resolveTexture(AN::Texture *src, UInt32 srcMipLevel,
                                AN::Texture *dst, UInt32 dstMipLevel, PixelFormat format) = 0;

    virtual void bindTexture(UInt32 binding, TextureID texID, ShaderStage stages, UInt32 set = 0) = 0;

    virtual void bindSampler(UInt32 binding, const SamplerDescriptor &sampler, ShaderStage stages, UInt32 set = 0) = 0;

    virtual DynamicVertexBuffer& getDynamicVertexBuffer() = 0;

    virtual void present(const AN::Presentable &presentable) = 0;

    virtual bool readTexture(void *outData, TextureID texID, int left, int top, UInt32 width, UInt32 height) = 0;

    virtual void setDepthBias(float bias, float slopeBias) = 0;
};


}// namespace AN

#endif//OJOIE_COMMANDBUFFER_HPP
