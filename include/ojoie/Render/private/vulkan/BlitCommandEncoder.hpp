//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_VK_BLITCOMMANDENCODER_HPP
#define OJOIE_VK_BLITCOMMANDENCODER_HPP

#include "Render/private/vulkan/CommandEncoder.hpp"
#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Buffer.hpp"

#include "Render/private/vulkan.hpp"



namespace AN::VK {

class BlitCommandEncoder : public CommandEncoder {

public:

    explicit BlitCommandEncoder(VkCommandBuffer commandBuffer)
        : CommandEncoder(commandBuffer) {

    }

    void copyBufferToImage(Buffer &buffer, uint64_t bufferOffset,
                           uint64_t bufferRowLength, uint64_t bufferImageHeight,
                           Image &image, uint32_t mipmapLevel,
                           int32_t xOffset, int32_t yOffset,
                           uint32_t width, uint32_t height,
                           VkImageAspectFlags aspectMask) {

        VkBufferImageCopy region{};
        region.bufferOffset = bufferOffset;
        region.bufferRowLength = bufferRowLength;
        region.bufferImageHeight = bufferImageHeight;
        region.imageSubresource.aspectMask = aspectMask;
        region.imageSubresource.mipLevel = mipmapLevel;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = image.getSubresource().arrayLayer;
        region.imageOffset = { .x = xOffset, .y = yOffset };
        region.imageExtent = {
                width,
                height,
                1
        };

        vkCmdCopyBufferToImage(_commandBuffer, buffer.vkBuffer(), image.vkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void copyBufferToBuffer(Buffer &srcBuffer, uint64_t srcOffset, Buffer &dstBuffer, uint64_t dstOffset, uint64_t size) {
        VkBufferCopy copyRegion;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;
        vkCmdCopyBuffer(_commandBuffer, srcBuffer.vkBuffer(), dstBuffer.vkBuffer(), 1, &copyRegion);
    }

    void generateMipmapsForImage(Image &image, uint32_t mipLevels) {

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image.vkImage();
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = (int32_t)image.getExtent().width;
        int32_t mipHeight = (int32_t)image.getExtent().height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(_commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            barrier.subresourceRange.baseMipLevel = i;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(_commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(_commandBuffer,
                           image.vkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image.vkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(_commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) {
                mipWidth /= 2;
            }
            if (mipHeight > 1) {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

    }
};

}

#endif//OJOIE_VK_BLITCOMMANDENCODER_HPP
