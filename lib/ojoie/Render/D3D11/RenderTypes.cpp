//
// Created by aojoie on 5/23/2023.
//

#include "Core/Exception.hpp"
#include "Render/private/D3D11/RenderTypes.hpp"

namespace AN::D3D11 {


DXGI_FORMAT toDXGIFormat(RenderTargetFormat format) {
    switch (format) {
        case kRTFormatRGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case kRTFormatDefault:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case kRTFormatNormalMap:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case kRTFormatDepth:
        case kRTFormatShadowMap:
            return DXGI_FORMAT_R16_TYPELESS;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

DXGI_FORMAT toDXGIFormat(PixelFormat format) {
    switch (format) {
        case kPixelFormatR8Unorm:
            return DXGI_FORMAT_R8_UNORM;
        case kPixelFormatR8Unorm_sRGB:
            return DXGI_FORMAT_R8_UNORM; /// no sRGB ?
        case kPixelFormatRG8Unorm_sRGB:
            return DXGI_FORMAT_R8G8_UNORM;
        case kPixelFormatRGBA8Unorm_sRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case kPixelFormatRGBA8Unorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case kPixelFormatBGRA8Unorm_sRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case kPixelFormatBGRA8Unorm:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case kPixelFormatRGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case kPixelFormatBC7_RGBAUnorm:
            return DXGI_FORMAT_BC7_UNORM;
        case kPixelFormatBC7_RGBAUnorm_sRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        case kPixelFormatBC1_RGBA:
            return DXGI_FORMAT_BC1_UNORM;
        case kPixelFormatBC1_RGBA_sRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case kPixelFormatBC2_RGBA:
            return DXGI_FORMAT_BC2_UNORM;
        case kPixelFormatBC2_RGBA_sRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case kPixelFormatBC3_RGBA:
            return DXGI_FORMAT_BC3_UNORM;
        case kPixelFormatBC3_RGBA_sRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case kPixelFormatBC4_RUnorm:
            return DXGI_FORMAT_BC4_UNORM;
        case kPixelFormatBC4_RSnorm:
            return DXGI_FORMAT_BC4_SNORM;

        case kPixelFormatBC5_RGUnorm:
            return DXGI_FORMAT_BC5_UNORM;
        case kPixelFormatBC5_RGSnorm:
            return DXGI_FORMAT_BC5_SNORM;

        case kPixelFormatBC6H_RGBFloat:
            return DXGI_FORMAT_BC6H_SF16;
        case kPixelFormatBC6H_RGBUfloat:
            return DXGI_FORMAT_BC6H_UF16;

        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

D3D11_TEXTURE_ADDRESS_MODE toDXGIFormat(SamplerAddressMode addressMode) {
    switch (addressMode) {
        case kSamplerAddressModeClampToZero:
        case kSamplerAddressModeClampToEdge:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        case kSamplerAddressModeMirrorClampToEdge:
            return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        case kSamplerAddressModeRepeat:
            return D3D11_TEXTURE_ADDRESS_WRAP;
        case kSamplerAddressModeMirrorRepeat:
            return D3D11_TEXTURE_ADDRESS_MIRROR;
        case kSamplerAddressModeClampToBorderColor:
            return D3D11_TEXTURE_ADDRESS_BORDER;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}
D3D11_COMPARISON_FUNC toDXGIFormat(CompareFunction function) {
    switch (function) {
        case kCompareFunctionNever:
            return D3D11_COMPARISON_NEVER;
        case kCompareFunctionLess:
            return D3D11_COMPARISON_LESS;
        case kCompareFunctionLEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case kCompareFunctionEqual:
            return D3D11_COMPARISON_EQUAL;
        case kCompareFunctionGEqual:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case kCompareFunctionGreater:
            return D3D11_COMPARISON_GREATER;
        case kCompareFunctionNotEqual:
            return D3D11_COMPARISON_NOT_EQUAL;
        case kCompareFunctionAlways:
            return D3D11_COMPARISON_ALWAYS;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

D3D11_FILTER toDXGIFormat(SamplerFilterMode filter, bool compare) {
    if (compare) {
        switch (filter) {
            case kSamplerFilterNearest:
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            case kSamplerFilterBilinear:
                return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            case kSamplerFilterTrilinear:
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            default:
                throw AN::Exception("Invalid Enum Value");
        }
    } else {
        switch (filter) {
            case kSamplerFilterNearest:
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case kSamplerFilterBilinear:
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case kSamplerFilterTrilinear:
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            default:
                throw AN::Exception("Invalid Enum Value");
        }
    }
}

D3D11_CULL_MODE toDXGIFormat(CullMode cullMode) {
    switch (cullMode) {
        case kCullModeNone:
            return D3D11_CULL_NONE;
        case kCullModeFront:
            return D3D11_CULL_FRONT;
        case kCullModeBack:
            return D3D11_CULL_BACK;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

D3D11_BLEND toD3DType(BlendFactor factor) {
    switch (factor) {
        case kBlendFactorZero:
            return D3D11_BLEND_ZERO;
        case kBlendFactorOne:
            return D3D11_BLEND_ONE;
        case kBlendFactorSourceColor:
            return D3D11_BLEND_SRC_COLOR;
        case kBlendFactorOneMinusSourceColor:
            return  D3D11_BLEND_INV_SRC_COLOR;
        case kBlendFactorSourceAlpha:
            return D3D11_BLEND_SRC_ALPHA;
        case kBlendFactorOneMinusSourceAlpha:
            return D3D11_BLEND_INV_SRC_ALPHA;
        case kBlendFactorDestinationColor:
            return D3D11_BLEND_DEST_COLOR;
        case kBlendFactorOneMinusDestinationColor:
            return D3D11_BLEND_INV_DEST_COLOR;
        case kBlendFactorDestinationAlpha:
            return D3D11_BLEND_DEST_ALPHA;
        case kBlendFactorOneMinusDestinationAlpha:
            return D3D11_BLEND_INV_DEST_ALPHA;
        case kBlendFactorSourceAlphaSaturated:
            return D3D11_BLEND_SRC_ALPHA_SAT;
        case kBlendFactorBlendColor:
            return D3D11_BLEND_BLEND_FACTOR;
        case kBlendFactorOneMinusBlendColor:
            return D3D11_BLEND_INV_BLEND_FACTOR;
        case kBlendFactorBlendAlpha:
            return D3D11_BLEND_BLEND_FACTOR;
        case kBlendFactorOneMinusBlendAlpha:
            return D3D11_BLEND_INV_BLEND_FACTOR;
        case kBlendFactorSource1Color:
            return D3D11_BLEND_SRC1_COLOR;
        case kBlendFactorOneMinusSource1Color:
            return D3D11_BLEND_INV_SRC1_COLOR;
        case kBlendFactorSource1Alpha:
            return D3D11_BLEND_SRC1_ALPHA;
        case kBlendFactorOneMinusSource1Alpha:
            return D3D11_BLEND_INV_SRC1_ALPHA;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}
D3D11_BLEND_OP toD3DType(BlendOperation op) {
    switch (op) {
        case kBlendOperationAdd:
            return D3D11_BLEND_OP_ADD;
        case kBlendOperationSubtract:
            return D3D11_BLEND_OP_SUBTRACT;
        case kBlendOperationReverseSubtract:
            return D3D11_BLEND_OP_REV_SUBTRACT;
        case kBlendOperationMin:
            return D3D11_BLEND_OP_MIN;
        case kBlendOperationMax:
            return D3D11_BLEND_OP_MAX;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

UINT8 toD3DType(ColorWriteMaskBits mask) {
    if (mask == kColorWriteMaskAll) {
        return D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    UINT8 ret = 0;
    if (mask & kColorWriteMaskRed) {
        ret |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (mask & kColorWriteMaskGreen) {
        ret |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (mask & kColorWriteMaskBlue) {
        ret |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (mask & kColorWriteMaskAlpha) {
        ret |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    return ret;
}

D3D11_PRIMITIVE_TOPOLOGY D3D11::toD3DType(PrimitiveType type) {
    switch (type) {
        case kPrimitiveTriangles:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case kPrimitiveQuads:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case kPrimitiveLines:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case kPrimitiveLineStrip:
            return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case kPrimitivePoints:
            return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

}