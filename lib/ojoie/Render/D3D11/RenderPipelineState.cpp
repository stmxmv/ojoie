//
// Created by aojoie on 5/24/2023.
//
#include "ojoie/Render/private/D3D11/RenderPipelineState.hpp"
#include "Render/private/D3D11/Device.hpp"

#include "Utility/Log.h"


namespace AN::D3D11 {

bool RenderPipelineState::init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                                          const PipelineReflection &reflection) {

    renderPipelineStateDescriptor = renderPipelineDescriptor;

    HRESULT hr;

    D3D_ASSERT(hr, GetD3D11Device()->CreateVertexShader(renderPipelineStateDescriptor.vertexFunction.code,
                                                        renderPipelineStateDescriptor.vertexFunction.size,
                                                        nullptr, &pVertexShader));

    D3D11SetDebugName(pVertexShader.Get(), std::format("VertexShader-{}-{}", (intptr_t)renderPipelineStateDescriptor.vertexFunction.code, renderPipelineStateDescriptor.vertexFunction.size).c_str());

    D3D_ASSERT(hr, GetD3D11Device()->CreatePixelShader(renderPipelineStateDescriptor.fragmentFunction.code,
                                                       renderPipelineStateDescriptor.fragmentFunction.size,
                                                       nullptr, &pPixelShader));

    D3D11SetDebugName(pPixelShader.Get(), std::format("VertexShader-{}-{}", (intptr_t)renderPipelineStateDescriptor.vertexFunction.code, renderPipelineStateDescriptor.vertexFunction.size).c_str());

    inputSig = GetVertexInputLayouts().getShaderInputSignature((void *)renderPipelineStateDescriptor.vertexFunction.code,
                                                               renderPipelineStateDescriptor.vertexFunction.size);

    ANAssert(inputSig);

    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = toDXGIFormat(renderPipelineStateDescriptor.cullMode);
    rasterizerDesc.FrontCounterClockwise = TRUE;
    rasterizerDesc.MultisampleEnable = renderPipelineDescriptor.rasterSampleCount > 1;
    rasterizerDesc.ScissorEnable = TRUE;
    rasterizerDesc.DepthClipEnable = TRUE;


    D3D_ASSERT(hr, GetD3D11Device()->CreateRasterizerState(&rasterizerDesc, &rasterizerState));


    D3D11_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = renderPipelineStateDescriptor.alphaToCoverageEnabled;

    for (int i = 0; i < renderPipelineStateDescriptor.colorAttachments.size(); ++i) {
        if (i + 1 > std::size(blendDesc.RenderTarget)) {
            AN_LOG(Error, "Error colorAttachment size");
            break;
        }

        blendDesc.RenderTarget[i].BlendEnable = renderPipelineStateDescriptor.colorAttachments[i].blendingEnabled;
        blendDesc.RenderTarget[i].SrcBlend = toD3DType(renderPipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor);
        blendDesc.RenderTarget[i].DestBlend = toD3DType(renderPipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor);
        blendDesc.RenderTarget[i].SrcBlendAlpha = toD3DType(renderPipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor);
        blendDesc.RenderTarget[i].DestBlendAlpha = toD3DType(renderPipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor);
        blendDesc.RenderTarget[i].BlendOp = toD3DType(renderPipelineDescriptor.colorAttachments[i].rgbBlendOperation);
        blendDesc.RenderTarget[i].BlendOpAlpha = toD3DType(renderPipelineDescriptor.colorAttachments[i].alphaBlendOperation);
        blendDesc.RenderTarget[i].RenderTargetWriteMask = toD3DType((ColorWriteMaskBits)renderPipelineStateDescriptor.colorAttachments[i].writeMask);
    }

    D3D_ASSERT(hr, GetD3D11Device()->CreateBlendState(&blendDesc, &blendState));

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = renderPipelineDescriptor.depthStencilDescriptor.depthTestEnabled;
    depthStencilDesc.DepthWriteMask = renderPipelineStateDescriptor.depthStencilDescriptor.depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = toDXGIFormat(renderPipelineDescriptor.depthStencilDescriptor.depthCompareFunction);
    depthStencilDesc.StencilEnable = FALSE;

    D3D_ASSERT(hr, GetD3D11Device()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState));

    return true;
}

void RenderPipelineState::deinit() {
    pVertexShader.Reset();
    pPixelShader.Reset();
    rasterizerState.Reset();
    blendState.Reset();
    depthStencilState.Reset();
}

}