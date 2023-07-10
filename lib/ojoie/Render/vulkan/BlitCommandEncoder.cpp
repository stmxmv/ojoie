//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/BlitCommandEncoder.hpp"
#include "Render/BlitCommandEncoder.hpp"

#include "Render/Texture.hpp"

#include <algorithm>
#include <limits>

#include "Template/Access.hpp"


namespace AN {

namespace  {

struct BufferImplTag : Access::TagBase<BufferImplTag> {};

}

template struct Access::Accessor<BufferImplTag, &RC::Buffer::impl>;

}

namespace AN::RC {

inline static VkPipelineStageFlags toVkPipelineStageFlags(PipelineStageFlag flag) {
    VkPipelineStageFlags ret{};
    if ((flag & PipelineStageFlag::VertexInput) != PipelineStageFlag::None) {
        ret |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if ((flag & PipelineStageFlag::VertexShader) != PipelineStageFlag::None) {
        ret |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if ((flag & PipelineStageFlag::Transfer) != PipelineStageFlag::None) {
        ret |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    return ret;
}

inline static VkAccessFlags toVkAccessFlags(PipelineAccessFlag flag) {
    VkAccessFlags ret{};
    if ((flag & PipelineAccessFlag::TransferRead) != PipelineAccessFlag::None) {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if ((flag & PipelineAccessFlag::TransferWrite) != PipelineAccessFlag::None) {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if ((flag & PipelineAccessFlag::ShaderRead) != PipelineAccessFlag::None) {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if ((flag & PipelineAccessFlag::ShaderWrite) != PipelineAccessFlag::None) {
        ret |= VK_ACCESS_SHADER_WRITE_BIT;
    }


    return ret;
}

BlitCommandEncoder::~BlitCommandEncoder() {
    if (shouldFree && impl) {
        delete (AN::VK::BlitCommandEncoder *)impl;
        impl = nullptr;
    }
}

void BlitCommandEncoder::copyBufferToTexture(Buffer &buffer,uint64_t bufferOffset,
                                             uint64_t bufferRowLength, uint64_t bufferImageHeight,
                                             Texture &texture, uint32_t mipmapLevel,
                                             int32_t xOffset, int32_t yOffset,
                                             uint32_t width, uint32_t height) {

    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;

    {
        // Prepare for transfer
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.srcAccessMask = 0;
        memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

//        encoder->imageBarrier(texture.getImageView(), memory_barrier);
    }

//    encoder->copyBufferToImage(*(VK::Buffer *)Access::get<BufferImplTag>(buffer), bufferOffset, bufferRowLength, bufferImageHeight,
//                               texture.getImage(), mipmapLevel, xOffset, yOffset, width, height, texture.getImageView().getSubresourceRange().aspectMask);

    {
        // Prepare for fragmen shader
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memory_barrier.srcAccessMask = 0;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

//        encoder->imageBarrier(texture.getImageView(), memory_barrier);
    }

}

void BlitCommandEncoder::generateMipmapsForTexture(Texture &texture, uint32_t mipmapLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
//    vkGetPhysicalDeviceFormatProperties(texture.getImage().getDevice().vkPhysicalDevice(), texture.getImage().getFormat(), &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        ANLog("texture image format does not support linear blitting!");
        return;
    }

    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    if (mipmapLevels == std::numeric_limits<uint32_t>::max()) {
//        mipmapLevels = (uint32_t)(std::floor(std::log2(std::max(texture.getImage().getExtent().width, texture.getImage().getExtent().height)))) + 1;
    }
//    mipmapLevels = std::min(mipmapLevels, texture.getImage().getSubresource().mipLevel);
//    encoder->generateMipmapsForImage(texture.getImage(), mipmapLevels);
}

void BlitCommandEncoder::copyBufferToBuffer(Buffer &srcBuffer, uint64_t srcOffset, Buffer &dstBuffer, uint64_t dstOffset, uint64_t size) {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    encoder->copyBufferToBuffer(
            *(VK::Buffer *)Access::get<BufferImplTag>(srcBuffer),
            srcOffset,
            *(VK::Buffer *)Access::get<BufferImplTag>(dstBuffer),
            dstOffset, size);
}

void BlitCommandEncoder::bufferMemoryBarrier(const Buffer &buffer, const BufferMemoryBarrier &memoryBarrier) {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    VK::BufferMemoryBarrier barrier;
    barrier.srcStageMask = toVkPipelineStageFlags(memoryBarrier.srcStageFlag);
    barrier.srcAccessMask = toVkAccessFlags(memoryBarrier.srcAccessMask);
    barrier.dstStageMask = toVkPipelineStageFlags(memoryBarrier.dstStageFlag);
    barrier.dstAccessMask = toVkAccessFlags(memoryBarrier.dstAccessMask);
    barrier.offset = memoryBarrier.offset;
    barrier.size = memoryBarrier.size;

    encoder->bufferBarrier(*(VK::Buffer *)Access::get<BufferImplTag>(buffer), barrier);
}

}