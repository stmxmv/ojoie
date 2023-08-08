//
// Created by aojoie on 5/24/2023.
//

#pragma once

#include <ojoie/Render/CommandBuffer.hpp>
#include <ojoie/Render/CommandPool.hpp>
#include <ojoie/Render/private/D3D11/Common.hpp>
#include <ojoie/Render/private/D3D11/RenderPipelineState.hpp>
#include <ojoie/Render/private/D3D11/VertexBuffer.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Math/Color.hpp>

namespace AN::D3D11 {

class CommandPool : public AN::CommandPool {

public:

    CommandPool();

    virtual CommandBuffer *newCommandBuffer() override;

    virtual void reset() override;

    virtual void waitAllCompleted() override;

    virtual void deinit() override;
};

CommandPool &GetCommandPool();

struct ImmediateVertexD3D11 {
    Vector3f	vertex;
    Vector3f	normal;
    ColorRGBA32	color;
    Vector3f	texCoords[8];
};

struct ImmediateModeD3D11 {
    std::vector<ImmediateVertexD3D11>	m_Vertices;
    ImmediateVertexD3D11				m_Current;
    PrimitiveType					    m_Mode;
    ComPtr<ID3D11Buffer>			    m_VB;
    int									m_VBUsedBytes = 0;
    int									m_VBStartVertex = 0;
    bool								m_HadColor;

    ~ImmediateModeD3D11();
    void Cleanup();
    void Invalidate();
};

class CommandBuffer : public AN::CommandBuffer {

    ID3D11DeviceContext       *context;
    ID3DUserDefinedAnnotation *annotation;
    D3D11_RASTERIZER_DESC      rasterizerDesc;

    ComPtr<ID3D11RasterizerState> rasterizerState;

    D3D11::RenderPipelineState *pipelineState;

    ImmediateModeD3D11 m_Imm;

    DynamicVertexBuffer m_DynamicVBO;

    void setRasterizerState();

    bool immediateEndSetup();
    void immediateEndDraw();

    friend class CommandPool;
public:

    void deinit();

    ID3D11DeviceContext *getContext() const { return context; }

    ID3D11InputLayout *getVertexInputLayout(const ChannelInfoArray& channels);

    virtual void submit() override;
    virtual void waitUntilCompleted() override;
    virtual bool isCompleted() override;
    virtual void reset() override;

    virtual void debugLabelBegin(const char *name, Vector4f color) override;
    virtual void debugLabelEnd() override;
    virtual void debugLabelInsert(const char *name, Vector4f color) override;

    virtual void beginRenderPass(UInt32 width, UInt32 height, RenderPass &renderPass, std::span<const RenderTarget *> renderTargets, std::span<ClearValue> clearValues) override;

    virtual void beginRenderPass(UInt32 width, UInt32 height, UInt32 samples,
                                 std::span<const AttachmentDescriptor> attachments,
                                 int depthAttachmentIndex) override;

    virtual void nextSubPass() override;
    virtual void endRenderPass() override;

    virtual void setViewport(const Viewport &viewport) override;
    virtual void setScissor(const ScissorRect &scissorRect) override;
    virtual void setCullMode(CullMode cullMode) override;
    virtual void setDepthBias(float bias, float slopeBias) override;

    virtual void setRenderPipelineState(AN::RenderPipelineState &renderPipelineState) override;

    virtual void pushConstants(uint32_t offset, uint32_t size, const void *data) override;

    virtual void drawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) override;
    virtual void draw(uint32_t count) override;

    virtual void textureBarrier(const TextureBarrier &textureBarrier) override;
    virtual void textureBarrier(std::span<const TextureBarrier> textureBarriers) override;

    virtual void blitTexture(TextureID srcTexID, Int32 srcWidth, Int32 srcHeight,
                             UInt32 srcMipLevel, TextureID dstTexID, Int32 dstWidth,
                             Int32 dstHeight, UInt32 dstMipLevel, TextureAspectFlags aspectMask, BlitFilter filter) override;

    virtual void present(const Presentable &presentable) override;

    virtual void bindTexture(UInt32 binding, TextureID texID, ShaderStage stages, UInt32 set) override;

    virtual void bindSampler(UInt32 binding, const SamplerDescriptor &sampler,
                             ShaderStage stages, UInt32 set) override;

    virtual void blitTexture(AN::Texture *tex, RenderTarget *target) override;

    virtual void blitTexture(AN::Texture *tex, RenderTarget *target, Material *material, int pass = 0) override;

    virtual void clearRenderTarget(RenderTarget *target, const Vector4f &color = { 0.f, 0.f, 0.f, 1.f }) override;

    virtual void resolveTexture(AN::Texture *src, UInt32 srcMipLevel,
                                AN::Texture *dst, UInt32 dstMipLevel, PixelFormat format) override;

    virtual bool readTexture(void *outData, TextureID texID, int left, int top, UInt32 width, UInt32 height) override;

    virtual AN::DynamicVertexBuffer& getDynamicVertexBuffer() override { return m_DynamicVBO; }

    virtual void immediateVertex(float x, float y, float z) override;

    virtual void immediateNormal(float x, float y, float z) override;

    virtual void immediateColor(float r, float g, float b, float a) override;

    virtual void immediateTexCoordAll(float x, float y, float z) override;

    virtual void immediateTexCoord(int unit, float x, float y, float z) override;

    virtual void immediateBegin(PrimitiveType type) override;

    virtual void immediateEnd() override;
};


}// namespace AN::D3D11
