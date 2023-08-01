//
// Created by aojoie on 5/24/2023.
//

#include "Render/private/D3D11/CommandBuffer.hpp"
#include "Misc/ResourceManager.hpp"
#include "Render/Material.hpp"
#include "Render/QualitySettings.hpp"
#include "Render/RenderPipelineState.hpp"
#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/Layer.hpp"
#include "Render/private/D3D11/RenderPass.hpp"
#include "Render/private/D3D11/RenderPipelineState.hpp"
#include "Render/private/D3D11/RenderTypes.hpp"
#include "Render/private/D3D11/Rendertarget.hpp"
#include "Render/private/D3D11/TextureManager.hpp"
#include "Render/private/D3D11/UniformBuffers.hpp"
#include "Utility/Log.h"
#include "Utility/win32/Unicode.hpp"

namespace AN::D3D11 {

static CommandBuffer gCommandBuffer;
static constexpr int kMaxImmediateVerticesPerDraw = 8192;

CommandPool::CommandPool() {
    gCommandBuffer.context = GetD3D11Context();
    gCommandBuffer.annotation = GetD3D11Annotation();
}

AN::CommandBuffer *CommandPool::newCommandBuffer() {
    gCommandBuffer.context = GetD3D11Context();
    gCommandBuffer.annotation = GetD3D11Annotation();
    gCommandBuffer.reset();
    return (AN::CommandBuffer *)&gCommandBuffer;
}

void CommandPool::reset() {
    gCommandBuffer.reset();
}

void CommandPool::waitAllCompleted() {}

void CommandPool::deinit() {
    gCommandBuffer.deinit();
}

CommandPool &GetCommandPool() {
    static AN::D3D11::CommandPool commandPool;
    return commandPool;
}

void CommandBuffer::deinit() {
    rasterizerState.Reset();
    m_Imm.Cleanup();
    m_DynamicVBO.deinit();
}

void CommandBuffer::submit() {}

void CommandBuffer::waitUntilCompleted() {}

bool CommandBuffer::isCompleted() {
    return true;
}

void CommandBuffer::reset() {
    pipelineState = nullptr;
    GetUniformBuffers().reset();
    m_Imm.Invalidate();
    context->ClearState();
}

void CommandBuffer::debugLabelBegin(const char *name, Vector4f color) {
    bool emiting = false;
#ifdef AN_DEBUG
    /// always emit debug label in debug build
    emiting = true;
#else
    if (IsEmittingDebugLabel()) {
        emiting = true;
    }
#endif
    if (emiting) {
        std::wstring wName = Utf8ToWide(name);
        annotation->BeginEvent(wName.c_str());
    }
}

void CommandBuffer::debugLabelEnd() {
    bool emiting = false;
#ifdef AN_DEBUG
    /// always emit debug label in debug build
    emiting = true;
#else
    if (IsEmittingDebugLabel()) {
        emiting = true;
    }
#endif
    if (emiting) {
        annotation->EndEvent();
    }
}

void CommandBuffer::debugLabelInsert(const char *name, Vector4f color) {
    bool emiting = false;
#ifdef AN_DEBUG
    /// always emit debug label in debug build
    emiting = true;
#else
    if (IsEmittingDebugLabel()) {
        emiting = true;
    }
#endif
    if (emiting) {
        std::wstring wName = Utf8ToWide(name);
        annotation->SetMarker(wName.c_str());
    }
}

void CommandBuffer::beginRenderPass(UInt32 width, UInt32 height,
                                    AN::RenderPass &renderPass,
                                    std::span<const AN::RenderTarget *> renderTargets,
                                    std::span<ClearValue> clearValues) {
    D3D11::RenderPass *rp = (D3D11::RenderPass *)renderPass.getImpl();

    std::vector<ID3D11RenderTargetView *> rtvs;
    for (UInt32 index : rp->renderPassDescriptor.subpasses[0].colorAttachments) {
        D3D11::RenderTarget *target = (D3D11::RenderTarget *)renderTargets[index]->getImpl();
        rtvs.push_back(target->getRTV());

        if (rp->renderPassDescriptor.loadStoreInfos[index].loadOp == kAttachmentLoadOpClear) {
            context->ClearRenderTargetView(target->getRTV(), (float *)&clearValues[index]);
        }
    }

    ID3D11DepthStencilView *dsv = nullptr;
    if (rp->renderPassDescriptor.subpasses[0].depthStencilAttachment) {
        UInt32 index = rp->renderPassDescriptor.subpasses[0].depthStencilAttachment.value();
        D3D11::RenderTarget *target = (D3D11::RenderTarget *)renderTargets[index]->getImpl();
        dsv = target->getDSV();

        if (rp->renderPassDescriptor.loadStoreInfos[index].loadOp == kAttachmentLoadOpClear) {
            context->ClearDepthStencilView(target->getDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                           clearValues[index].depthStencil.depth,
                                           clearValues[index].depthStencil.stencil);
        }
    }

    context->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
}

void CommandBuffer::beginRenderPass(UInt32 width, UInt32 height, UInt32 samples,
                                    std::span<const AttachmentDescriptor> attachments,
                                    int depthAttachmentIndex) {

    std::vector<ID3D11RenderTargetView *> rtvs;

    for (int i = 0; i < attachments.size(); ++i) {
        if (i == depthAttachmentIndex) continue;

        D3D11::RenderTarget *target = (D3D11::RenderTarget *)attachments[i].loadStoreTarget->getImpl();
        rtvs.push_back(target->getRTV());

        if (attachments[i].loadOp == kAttachmentLoadOpClear) {
            context->ClearRenderTargetView(target->getRTV(), (float *)&attachments[i].clearColor);
        }
    }

    ID3D11DepthStencilView *dsv = nullptr;
    if (depthAttachmentIndex >= 0 && depthAttachmentIndex < attachments.size()) {
        D3D11::RenderTarget *target = (D3D11::RenderTarget *)attachments[depthAttachmentIndex].loadStoreTarget->getImpl();
        dsv = target->getDSV();
        if (attachments[depthAttachmentIndex].loadOp == kAttachmentLoadOpClear) {
            context->ClearDepthStencilView(target->getDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                           attachments[depthAttachmentIndex].clearDepth,
                                           attachments[depthAttachmentIndex].clearStencil);
        }
    }

    context->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
}

void CommandBuffer::nextSubPass() {
    AN_LOG(Error, "Not support subpass");
}

void CommandBuffer::endRenderPass() {}

void CommandBuffer::setRasterizerState() {
    HRESULT hr;
    D3D_ASSERT(hr, GetD3D11Device()->CreateRasterizerState(&rasterizerDesc, &rasterizerState));
    context->RSSetState(rasterizerState.Get());
}

void CommandBuffer::setViewport(const Viewport &viewport) {
    D3D11_VIEWPORT vp;
    vp.TopLeftX = viewport.originX;
    vp.TopLeftY = viewport.originY;
    vp.Width = viewport.width;
    vp.Height = viewport.height;
    vp.MinDepth = viewport.znear;
    vp.MaxDepth = viewport.zfar;
    context->RSSetViewports(1, &vp);
}

void CommandBuffer::setScissor(const ScissorRect &scissorRect) {
    D3D11_RECT rect;
    rect.left = scissorRect.x;
    rect.top = scissorRect.y;
    rect.right = scissorRect.x + scissorRect.width;
    rect.bottom = scissorRect.y + scissorRect.height;
    context->RSSetScissorRects(1, &rect);
}

void CommandBuffer::setCullMode(CullMode cullMode) {
    D3D11_CULL_MODE mode = toDXGIFormat(cullMode);
    if (mode != rasterizerDesc.CullMode) {
        rasterizerDesc.CullMode = mode;
        setRasterizerState();
    }
}

ID3D11InputLayout *CommandBuffer::getVertexInputLayout(const ChannelInfoArray& channels) {
    return GetVertexInputLayouts().getVertexInputLayout(channels, pipelineState->getInputSignature());
}

void CommandBuffer::setRenderPipelineState(AN::RenderPipelineState &renderPipelineState) {
    D3D11::RenderPipelineState * state = (D3D11::RenderPipelineState *)renderPipelineState.getImpl();
    if (state != pipelineState) {
        pipelineState = state;
        context->VSSetShader(pipelineState->getVertexShader(), nullptr, 0);
        context->PSSetShader(pipelineState->getPixelShader(), nullptr, 0);
        context->RSSetState(pipelineState->getRasterizerState());
        context->OMSetBlendState(pipelineState->getBlendState(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(pipelineState->getDepthStencilState(), 1u);
    }
}

void CommandBuffer::pushConstants(uint32_t offset, uint32_t size, const void *data) {
    AN_LOG(Error, "Not support pushConstants");
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t indexOffset,
                                uint32_t vertexOffset) {
    GetUniformBuffers().updateBuffers(this);
    context->DrawIndexed(indexCount, indexOffset, vertexOffset);
#ifdef AN_DEBUG
    LogD3D11DebugMessage();
#endif
}

void CommandBuffer::draw(uint32_t count) {
    GetUniformBuffers().updateBuffers(this);
    context->Draw(count, 0);
#ifdef AN_DEBUG
    LogD3D11DebugMessage();
#endif
}

void CommandBuffer::textureBarrier(const TextureBarrier &textureBarrier) {}

void CommandBuffer::textureBarrier(std::span<const TextureBarrier> textureBarriers) {}

void CommandBuffer::blitTexture(TextureID srcTexID,
                                Int32 srcWidth, Int32 srcHeight,
                                UInt32 srcMipLevel, TextureID dstTexID,
                                Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                                TextureAspectFlags aspectMask, BlitFilter filter) {


}

[[noreturn]] void AbortOnD3D11Error();

void CommandBuffer::present(const AN::Presentable &presentable) {
    D3D11::Presentable &pre = (D3D11::Presentable &)presentable;
    UInt32 vSyncCount = GetQualitySettings().getCurrent().vSyncCount;
    HRESULT hr;
    if (vSyncCount > 0) {
        hr = pre._swapChain->Present(vSyncCount, 0);
    } else {
        hr = pre._swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING); // we don't want vsync
    }

    if (FAILED(hr)) {
        AbortOnD3D11Error();
    }
}

void CommandBuffer::bindTexture(UInt32 binding, TextureID texID, ShaderStage stages, UInt32 set) {
    auto tex =  GetTextureManager().getTexture(texID);
    if (stages == kShaderStageVertex) {
        context->VSSetShaderResources(binding, 1, tex->_srv.GetAddressOf());
    } else if (stages == kShaderStageFragment) {
        context->PSSetShaderResources(binding, 1, tex->_srv.GetAddressOf());
    }
}
void CommandBuffer::bindSampler(UInt32 binding, const SamplerDescriptor &samplerDescriptor,
                                ShaderStage stages, UInt32 set) {
    ID3D11SamplerState *sampler = GetTextureManager().getSampler(samplerDescriptor);
    if (stages == kShaderStageVertex) {
        context->VSSetSamplers(binding, 1, &sampler);
    } else if (stages == kShaderStageFragment) {
        context->PSSetSamplers(binding, 1, &sampler);
    }
}

void CommandBuffer::blitTexture(AN::Texture *tex, AN::RenderTarget *target, Material *s_BlitMaterial, int pass) {
    Size size = target->getSize();
    D3D11::RenderTarget *renderTarget = (D3D11::RenderTarget *)target->getImpl();
    ID3D11RenderTargetView *rtv = renderTarget->getRTV();

    reset();

    s_BlitMaterial->setTexture("_MainTex", tex);
    s_BlitMaterial->applyMaterial(this, pass);

    context->OMSetRenderTargets(1, &rtv, nullptr);

    setViewport({ .originX = 0.f, .originY = 0.f, .width = (float)size.width, .height = (float)size.height });
    setScissor({ .x = 0, .y = 0, .width = (int)size.width, .height = (int)size.height });

    immediateBegin(kPrimitiveQuads);
    immediateTexCoordAll(0.0f, 1.f, 0.0f);
    immediateVertex (-1.0f, -1.0f, 0.1f);
    immediateTexCoordAll (1.f, 1.f, 0.0f);
    immediateVertex (1.0f, -1.0f, 0.1f);
    immediateTexCoordAll (1.f,  0.f, 0.0f);
    immediateVertex (1.0f, 1.0f, 0.1f);
    immediateTexCoordAll (0.f,  0.f, 0.0f);
    immediateVertex (-1.0f, 1.0f, 0.1f);
    immediateEnd ();
#ifdef AN_DEBUG
    LogD3D11DebugMessage();
#endif
}

void CommandBuffer::clearRenderTarget(AN::RenderTarget *target, const Vector4f &color) {
    D3D11::RenderTarget *renderTarget = (D3D11::RenderTarget *)target->getImpl();
    ID3D11RenderTargetView *rtv = renderTarget->getRTV();
    context->ClearRenderTargetView(rtv, (float *)&color);
}

void CommandBuffer::blitTexture(AN::Texture *tex, AN::RenderTarget *target) {
    static Material* s_BlitMaterial = nullptr;
    static Shader* s_BlitShader = nullptr;
    if (!s_BlitMaterial) {
        s_BlitShader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "BlitCopy");
        ANAssert(s_BlitShader);
        s_BlitMaterial = NewObject<Material>();
        ANAssert(s_BlitMaterial->init(s_BlitShader, "BlitMaterial"));
    }

    blitTexture(tex, target, s_BlitMaterial);
}

void CommandBuffer::resolveTexture(AN::Texture *src, UInt32 srcMipLevel,
                                   AN::Texture *dst, UInt32 dstMipLevel,
                                   PixelFormat format) {
    auto srcTex =  GetTextureManager().getTexture(src->getTextureID());
    auto dstTex =  GetTextureManager().getTexture(dst->getTextureID());
    context->ResolveSubresource(dstTex->_texture.Get(), dstMipLevel, srcTex->_texture.Get(), srcMipLevel, toDXGIFormat(format));
}

ImmediateModeD3D11::~ImmediateModeD3D11() {
    ANAssert(m_VB == nullptr);
}

void ImmediateModeD3D11::Cleanup() {
    m_VBUsedBytes = 0;
    m_VB.Reset();
}

void ImmediateModeD3D11::Invalidate() {
    m_Vertices.clear();
    memset( &m_Current, 0, sizeof(m_Current) );
    m_HadColor = false;
}


void CommandBuffer::immediateVertex(float x, float y, float z) {
    // If the current batch is becoming too large, internally end it and begin it again.
    size_t currentSize = m_Imm.m_Vertices.size();
    if (currentSize >= kMaxImmediateVerticesPerDraw - 4) {
        PrimitiveType mode = m_Imm.m_Mode;
        // For triangles, break batch when multiple of 3's is reached.
        if (mode == kPrimitiveTriangles && currentSize % 3 == 0) {
            bool hadColor = m_Imm.m_HadColor;
            immediateEnd();
            immediateBegin(mode);
            m_Imm.m_HadColor = hadColor;
        }
        // For other primitives, break on multiple of 4's.
        // NOTE: This won't quite work for triangle strips, but we'll just pretend
        // that will never happen.
        else if (mode != kPrimitiveTriangles && currentSize % 4 == 0) {
            bool hadColor = m_Imm.m_HadColor;
            immediateEnd();
            immediateBegin(mode);
            m_Imm.m_HadColor = hadColor;
        }
    }
    Vector3f &vert = m_Imm.m_Current.vertex;
    vert.x         = x;
    vert.y         = y;
    vert.z         = z;
    m_Imm.m_Vertices.push_back(m_Imm.m_Current);
}

void CommandBuffer::immediateNormal(float x, float y, float z) {
    m_Imm.m_Current.normal.x = x;
    m_Imm.m_Current.normal.y = y;
    m_Imm.m_Current.normal.z = z;
}

void CommandBuffer::immediateColor(float r, float g, float b, float a) {
    m_Imm.m_Current.color.set(ColorRGBAf(r,g,b,a));
    m_Imm.m_HadColor = true;
}
void CommandBuffer::immediateTexCoordAll(float x, float y, float z) {
    for( int i = 0; i < 8; ++i ) {
        Vector3f& uv = m_Imm.m_Current.texCoords[i];
        uv.x = x;
        uv.y = y;
        uv.z = z;
    }
}

void CommandBuffer::immediateTexCoord(int unit, float x, float y, float z) {
    if( unit < 0 || unit >= 8 ) {
        ANAssert("Invalid unit for texcoord");
        return;
    }
    Vector3f& uv = m_Imm.m_Current.texCoords[unit];
    uv.x = x;
    uv.y = y;
    uv.z = z;
}

void CommandBuffer::immediateBegin(PrimitiveType type) {
    m_Imm.m_Mode = type;
    m_Imm.m_Vertices.clear();
    m_Imm.m_HadColor = false;
}

bool CommandBuffer::immediateEndSetup() {
    if( m_Imm.m_Vertices.empty() )
        return false;

    HRESULT hr = S_OK;
    ID3D11DeviceContext* ctx = context;

    // vertex buffer
    const int kImmediateVBSize = kMaxImmediateVerticesPerDraw * sizeof(ImmediateVertexD3D11);
    if (!m_Imm.m_VB) {
        ID3D11Device* dev = GetD3D11Device();
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = kImmediateVBSize;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;
        D3D_ASSERT(hr, dev->CreateBuffer (&desc, NULL, &m_Imm.m_VB));
        D3D11SetDebugName(m_Imm.m_VB.Get(), "VertexBufferImmediate");
        m_Imm.m_VBUsedBytes = 0;
        m_Imm.m_VBStartVertex = 0;
    }

    const ImmediateVertexD3D11* vb = &m_Imm.m_Vertices[0];
    const int vertexCount = m_Imm.m_Vertices.size();
    const int vertexDataSize = vertexCount * sizeof(vb[0]);
    D3D11_MAPPED_SUBRESOURCE mapped;

    if (m_Imm.m_VBUsedBytes + vertexDataSize > kImmediateVBSize)
    {
        D3D_ASSERT(hr, ctx->Map (m_Imm.m_VB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        m_Imm.m_VBUsedBytes = 0;
    }
    else
    {
        D3D_ASSERT(hr, ctx->Map (m_Imm.m_VB.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
    }
    m_Imm.m_VBStartVertex = m_Imm.m_VBUsedBytes / sizeof(vb[0]);

    memcpy ((UInt8*)mapped.pData + m_Imm.m_VBUsedBytes, vb, vertexDataSize);
    ctx->Unmap (m_Imm.m_VB.Get(), 0);
    m_Imm.m_VBUsedBytes += vertexDataSize;

    UINT strides = sizeof(vb[0]);
    UINT offsets = 0;
    ctx->IASetVertexBuffers(0, 1, m_Imm.m_VB.GetAddressOf(), &strides, &offsets);

    GetUniformBuffers().updateBuffers(this);
    return true;
}

void CommandBuffer::immediateEndDraw() {
    ID3D11DeviceContext* ctx = GetD3D11Context();
    int vertexCount = m_Imm.m_Vertices.size();

    // vertex layout
    ID3D11InputLayout* inputLayout =  GetVertexInputLayouts().getImmVertexDecl(pipelineState->getInputSignature());
    if (inputLayout) {
        ctx->IASetInputLayout(inputLayout);
        ctx->IASetPrimitiveTopology(toD3DType(m_Imm.m_Mode));
        // draw
        switch (m_Imm.m_Mode) {
            case kPrimitiveTriangles:
                ctx->Draw(vertexCount, m_Imm.m_VBStartVertex);
                break;
            case kPrimitiveQuads:
                ctx->IASetIndexBuffer(m_DynamicVBO.getQuadsIB(), DXGI_FORMAT_R16_UINT, 0);
                ctx->DrawIndexed(vertexCount / 4 * 6, 0, m_Imm.m_VBStartVertex);
                break;
            case kPrimitiveLines:
                ctx->Draw(vertexCount, m_Imm.m_VBStartVertex);
                break;
            default:
                ANAssert(false && "ImmediateEnd: unknown draw mode");
        }
    }

    // clear vertices
    m_Imm.m_Vertices.clear();
}

void CommandBuffer::immediateEnd() {
    if (immediateEndSetup()) {
        immediateEndDraw();
    }
}

bool CommandBuffer::readTexture(void *outData, TextureID texID, int left, int top, UInt32 width, UInt32 height) {

    ComPtr<ID3D11Texture2D> stagingTex;
    D3D11_TEXTURE2D_DESC stagingDesc;
    stagingDesc.Width = width;
    stagingDesc.Height = height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;
    HRESULT hr = GetD3D11Device()->CreateTexture2D (&stagingDesc, NULL, &stagingTex);

    if (FAILED(hr))
        return false;

    auto tex =  GetTextureManager().getTexture(texID);
    D3D11_TEXTURE2D_DESC rtDesc;
    tex->_texture->GetDesc(&rtDesc);

    D3D11SetDebugName(stagingTex.Get(), std::format("Read-Texture2D-{}x{}", width, height));

    D3D11_BOX srcBox;
    srcBox.left = left;
    srcBox.right = left + width;
    srcBox.top = top;
    srcBox.bottom = top + height;
    srcBox.front = 0;
    srcBox.back = 1;
    context->CopySubresourceRegion(stagingTex.Get(), 0, 0, 0, 0, tex->_texture.Get(), 0, &srcBox);

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map (stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr))
        return false;

    const UInt8* src = (const UInt8*)mapped.pData;

    /// read as ARGB
    for (int y = height-1; y >= 0; --y)
    {
        const UInt32* srcPtr = (const UInt32*)src;
        UInt32* dstPtr = (UInt32*)((char *)outData + width * y * 4);
        for (int x = 0; x < width; ++x)
        {
            UInt32 abgrCol = *srcPtr;
            UInt32 bgraCol = ((abgrCol & 0x00FFFFFF) << 8) | ((abgrCol&0xFF000000) >> 24);
            *dstPtr = bgraCol;
            ++srcPtr;
            ++dstPtr;
        }
        src += mapped.RowPitch;
    }

    context->Unmap(stagingTex.Get(), 0);

    return true;
}

}