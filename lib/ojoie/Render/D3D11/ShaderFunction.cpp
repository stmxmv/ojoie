//
// Created by aojoie on 5/24/2023.
//

#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/ShaderFunction.hpp"

#include "Utility/Log.h"


namespace AN::D3D11 {

bool ShaderFunction::init(const UInt8 *code, size_t size, AN::ShaderStage stage) {

    HRESULT hr;
    if (stage == kShaderStageVertex) {
        D3D_ASSERT(hr, GetD3D11Device()->CreateVertexShader(code, size, nullptr, &pVertexShader));
        D3D11SetDebugName(pVertexShader.Get(), std::format("VertexShader-{}-{}", (intptr_t)code, size).c_str());
    } else if (stage == kShaderStageFragment) {
        D3D_ASSERT(hr, GetD3D11Device()->CreatePixelShader(code, size, nullptr, &pPixelShader));
        D3D11SetDebugName(pPixelShader.Get(), std::format("VertexShader-{}-{}", (intptr_t)code, size).c_str());
    } else {
        AN_LOG(Error, "Unsupported shader stage");
        return false;
    }

    return true;
}

}