//
// Created by aojoie on 5/24/2023.
//

#pragma once

#include <ojoie/Render/private/D3D11/Common.hpp>

namespace AN::D3D11 {

class ShaderFunction : private NonCopyable {

    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;

public:

    ~ShaderFunction() {
        deinit();
    }

    bool init(const UInt8 *code, size_t size, ShaderStage stage);

    void deinit() {
        pVertexShader.Reset();
        pPixelShader.Reset();
    }

    ID3D11VertexShader *getVertexShader() const { return pVertexShader.Get(); }

    ID3D11PixelShader *getPixelShader() const { return pPixelShader.Get(); }
};


}
