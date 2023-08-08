//
// Created by aojoie on 5/23/2023.
//

#pragma once
#include <dxgiformat.h>
#include <d3d11.h>
#include <ojoie/Render/RenderTypes.hpp>

namespace AN::D3D11 {

DXGI_FORMAT toDXGIFormat(RenderTargetFormat format);
DXGI_FORMAT toDXGIFormat(PixelFormat format);

D3D11_TEXTURE_ADDRESS_MODE toDXGIFormat(SamplerAddressMode addressMode);

D3D11_COMPARISON_FUNC toDXGIFormat(CompareFunction function);

D3D11_FILTER toDXGIFormat(SamplerFilterMode filter, bool compare = false);

D3D11_CULL_MODE toDXGIFormat(CullMode cullMode);

D3D11_BLEND toD3DType(BlendFactor factor);

D3D11_BLEND_OP toD3DType(BlendOperation op);

UINT8 toD3DType(ColorWriteMaskBits mask);

D3D11_PRIMITIVE_TOPOLOGY toD3DType(PrimitiveType type);
}
