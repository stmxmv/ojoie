//
// Created by aojoie on 4/14/2023.
//

#include "Render/private/vulkan/RenderTypes.hpp"
#include "Render/private/vulkan/Device.hpp"
#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Utility/Log.h>

namespace AN::VK {


ShaderStageFlags to_ANRenderType(VkShaderStageFlags stage) {
    ShaderStageFlags ret = 0;

    if (stage & VK_SHADER_STAGE_VERTEX_BIT) {
        ret |= kShaderStageVertex;
    }
    if (stage & VK_SHADER_STAGE_FRAGMENT_BIT) {
        ret |= kShaderStageFragment;
    }

    return ret;
}

VkShaderStageFlagBits to_VkRenderType(ShaderStageFlagBits stage) {
    VkShaderStageFlags ret = 0;

    if (stage & kShaderStageVertex) {
        ret |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (stage & kShaderStageFragment) {
        ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    return (VkShaderStageFlagBits)ret;
}

VkSamplerAddressMode toVkRenderType(SamplerAddressMode addressMode) {
    switch (addressMode) {
        case kSamplerAddressModeRepeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case kSamplerAddressModeMirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case kSamplerAddressModeClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case kSamplerAddressModeClampToBorderColor:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case kSamplerAddressModeMirrorClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        case kSamplerAddressModeClampToZero:
            ANLog("Not support SamplerAddressMode ClampToZero in Vulkan, use Repeat instead");
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        default:
            throw AN::Exception("Invalid enum value");
    }
}

VkBorderColor toVkRenderType(SamplerBorderColor borderColor) {
    switch (borderColor) {
        case kSamplerBorderColorTransparentBlack:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case kSamplerBorderColorOpaqueBlack:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case kSamplerBorderColorOpaqueWhite:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        default:
            throw AN::Exception("Invalid enum value");
    }
}

VkFilter toVkRenderType(SamplerMinMagFilter filter) {
    switch (filter) {
        case kSamplerMinMagFilterNearest:
            return VK_FILTER_NEAREST;
        case kSamplerMinMagFilterLinear:
            return VK_FILTER_LINEAR;
        default:
            throw AN::Exception("Invalid enum value");
    }
}

VkSamplerMipmapMode toVkRenderType(SamplerMipFilter mipFilter) {
    switch (mipFilter) {
        case kSamplerMipFilterNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case kSamplerMipFilterLinear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case kSamplerMipFilterNotMipmapped:
            ANLog("Not support SamplerMipFilter NotMipmapped in Vulkan, use Linear instead");
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default:
            throw AN::Exception("Invalid enum value");
    }
}

VkCompareOp toVkRenderType(CompareFunction function) {
    ANAssert(function < kCompareFunctionCount);
    switch (function) {
        case kCompareFunctionNever:
            return VK_COMPARE_OP_NEVER;
        case kCompareFunctionLess:
            return VK_COMPARE_OP_LESS;
        case kCompareFunctionEqual:
            return VK_COMPARE_OP_EQUAL;
        case kCompareFunctionLEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case kCompareFunctionGreater:
            return VK_COMPARE_OP_GREATER;
        case kCompareFunctionNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case kCompareFunctionGEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case kCompareFunctionAlways:
            return VK_COMPARE_OP_ALWAYS;
        default:
            throw AN::Exception("Invalid enum value");
    }
}


VkAttachmentLoadOp toVkRenderType(AttachmentLoadOp op) {
    switch (op) {
        case kAttachmentLoadOpLoad:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case kAttachmentLoadOpClear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case kAttachmentLoadOpDontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        default:
            throw AN::Exception("Invalid enum value");
    }
}
VkAttachmentStoreOp toVkRenderType(AttachmentStoreOp op) {
    switch (op) {
        case kAttachmentStoreOpStore:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case kAttachmentStoreOpDontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case kAttachmentStoreOpNone:
            return VK_ATTACHMENT_STORE_OP_NONE;
        default:
            throw AN::Exception("Invalid enum value");
    }
}

VkFormat toVkRenderType(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case kPixelFormatR8Unorm:
            return VK_FORMAT_R8_UNORM;
        case kPixelFormatR8Unorm_sRGB:
            return VK_FORMAT_R8_SRGB;
        case kPixelFormatRG8Unorm_sRGB:
            return VK_FORMAT_R8G8_SRGB;
        case kPixelFormatRGBA8Unorm_sRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case kPixelFormatRGBA8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            throw AN::Exception("Invalid enum value");
    }
}


VkFormat toVkRenderType(RenderTargetFormat format) {
    switch (format){
        case kRTFormatSwapchain:
            return GetDevice().getVkSurfaceFormatKHR().format;
        case kRTFormatDepth:
            /// default depth
            return VK_FORMAT_D32_SFLOAT;
        case kRTFormatDefault:
            /// default color image
            return VK_FORMAT_B8G8R8A8_UNORM;
        case kRTFormatShadowMap:
            /// TODO
            break;
        case kRTFormatNormalMap:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case kRTFormatCount:
            break;
        default:
            throw AN::Exception("Invalid enum value");
    }
    throw AN::Exception("Invalid enum value");
}

VkCullModeFlags toVkRenderType(CullMode cullMode) {
    switch (cullMode) {
        case kCullModeBack:
            return VK_CULL_MODE_BACK_BIT;
        case kCullModeFront:
            return VK_CULL_MODE_FRONT_BIT;
        case kCullModeNone:
            return VK_CULL_MODE_NONE;
    }
    throw AN::Exception("Invalid Enum Value");
}

}// namespace AN::VK