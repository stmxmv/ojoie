//
// Created by aojoie on 4/14/2023.
//

#ifndef OJOIE_VK_RENDERTYPES_HPP
#define OJOIE_VK_RENDERTYPES_HPP

#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Render/RenderTypes.hpp>

namespace AN::VK {

ShaderStageFlags to_ANRenderType(VkShaderStageFlags stage);

VkShaderStageFlagBits to_VkRenderType(ShaderStageFlagBits stage);

VkSamplerAddressMode toVkRenderType(SamplerAddressMode addressMode);

VkBorderColor toVkRenderType(SamplerBorderColor borderColor);

VkFilter toVkRenderType(SamplerMinMagFilter filter);

VkSamplerMipmapMode toVkRenderType(SamplerMipFilter mipFilter);

VkCompareOp toVkRenderType(CompareFunction function);

VkFormat toVkRenderType(PixelFormat pixelFormat);

VkAttachmentLoadOp toVkRenderType(AttachmentLoadOp op);

VkAttachmentStoreOp toVkRenderType(AttachmentStoreOp op);

VkFormat toVkRenderType(RenderTargetFormat format);

VkCullModeFlags toVkRenderType(CullMode cullMode);

}

#endif//OJOIE_VK_RENDERTYPES_HPP
