//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/BlitCommandEncoder.hpp"
#include "Render/BlitCommandEncoder.hpp"

#include "Render/Texture.hpp"

#include <algorithm>
#include <limits>

namespace AN::RC {

BlitCommandEncoder::~BlitCommandEncoder() {
    if ((flag & BlitCommandEncoderFlag::ShouldFree) != BlitCommandEncoderFlag::None && impl) {
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

        encoder->imageBarrier(texture.getImageView(), memory_barrier);
    }

    encoder->copyBufferToImage(**(VK::Buffer **)&buffer, bufferOffset, bufferRowLength, bufferImageHeight,
                               texture.getImage(), mipmapLevel, xOffset, yOffset, width, height, texture.getImageView().getSubresourceRange().aspectMask);

    {
        // Prepare for fragmen shader
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memory_barrier.srcAccessMask = 0;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        encoder->imageBarrier(texture.getImageView(), memory_barrier);
    }

}

void BlitCommandEncoder::generateMipmapsForTexture(Texture &texture, uint32_t mipmapLevels) {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    if (mipmapLevels == std::numeric_limits<uint32_t>::max()) {
        mipmapLevels = (uint32_t)(std::floor(std::log2(std::max(texture.getImage().getExtent().width, texture.getImage().getExtent().height)))) + 1;
    }
    mipmapLevels = std::min(mipmapLevels, texture.getImage().getSubresource().mipLevel);
    encoder->generateMipmapsForImage(texture.getImage(), mipmapLevels);
}

void BlitCommandEncoder::copyBufferToBuffer(Buffer &srcBuffer, uint64_t srcOffset, Buffer &dstBuffer, uint64_t dstOffset, uint64_t size) {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    encoder->copyBufferToBuffer(**(VK::Buffer **)&srcBuffer, srcOffset, **(VK::Buffer **)&dstBuffer, dstOffset, size);
}

void BlitCommandEncoder::submit() {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    encoder->submit();
}

void BlitCommandEncoder::waitComplete() {
    VK::BlitCommandEncoder *encoder = (VK::BlitCommandEncoder *)impl;
    encoder->waitComplete();
}

}