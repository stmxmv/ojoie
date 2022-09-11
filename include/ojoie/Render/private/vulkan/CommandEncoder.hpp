//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_COMMANDENCODER_HPP
#define OJOIE_COMMANDENCODER_HPP

#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan.hpp"

namespace AN::VK {

struct ImageMemoryBarrier {
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
};

class CommandEncoder {
protected:

    VkCommandBuffer _commandBuffer;
public:

    CommandEncoder(VkCommandBuffer commandBuffer) : _commandBuffer(commandBuffer) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }

    void imageBarrier(const ImageView &imageView, const ImageMemoryBarrier &memoryBarrier) {

        VkImageMemoryBarrier image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image_memory_barrier.oldLayout        = memoryBarrier.oldLayout;
        image_memory_barrier.newLayout        = memoryBarrier.newLayout;
        image_memory_barrier.image            = imageView.getImage()->vkImage();
        image_memory_barrier.subresourceRange = imageView.getSubresourceRange();
        image_memory_barrier.srcAccessMask    = memoryBarrier.srcAccessMask;
        image_memory_barrier.dstAccessMask    = memoryBarrier.dstAccessMask;

        VkPipelineStageFlags src_stage_mask = memoryBarrier.srcStageMask;
        VkPipelineStageFlags dst_stage_mask = memoryBarrier.dstStageMask;

        vkCmdPipelineBarrier(
                _commandBuffer,
                src_stage_mask,
                dst_stage_mask,
                0,
                0, nullptr,
                0, nullptr,
                1,
                &image_memory_barrier);
    }


};

}

#endif//OJOIE_COMMANDENCODER_HPP
