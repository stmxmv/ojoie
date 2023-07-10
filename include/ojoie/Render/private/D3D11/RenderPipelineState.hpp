//
// Created by aojoie on 5/24/2023.
//

#pragma once

#include <ojoie/Render/RenderPipelineState.hpp>
#include <ojoie/Render/private/D3D11/ShaderFunction.hpp>

#include <ojoie/Render/private/D3D11/VertexInputLayouts.hpp>

namespace AN::D3D11 {


class RenderPipelineState : public RenderPipelineStateImpl {

    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;

    ComPtr<ID3D11RasterizerState>   rasterizerState;
    ComPtr<ID3D11BlendState>        blendState;
    ComPtr<ID3D11DepthStencilState> depthStencilState;

    const InputSignatureD3D11* inputSig;

    RenderPipelineStateDescriptor renderPipelineStateDescriptor;

public:

    virtual bool init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                      const PipelineReflection            &reflection) override;


    virtual void deinit() override;

    ID3D11BlendState *getBlendState() const { return blendState.Get(); }

    ID3D11DepthStencilState *getDepthStencilState() const { return depthStencilState.Get(); }

    ID3D11RasterizerState *getRasterizerState() const { return rasterizerState.Get(); }

    ID3D11VertexShader *getVertexShader() const { return pVertexShader.Get(); }

    ID3D11PixelShader *getPixelShader() const { return pPixelShader.Get(); }

    const InputSignatureD3D11 *getInputSignature() const { return inputSig; }
};

}// namespace AN::D3D11