//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_COMMANDENCODER_HPP
#define OJOIE_COMMANDENCODER_HPP

#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan/Buffer.hpp"
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

struct BufferMemoryBarrier {
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkDeviceSize       offset;
    VkDeviceSize       size;
};

class CommandEncoder {
protected:

    VkCommandBuffer _commandBuffer;
public:

    CommandEncoder(VkCommandBuffer commandBuffer) : _commandBuffer(commandBuffer) {}

    void imageBarrier(const ImageView &imageView, const ImageMemoryBarrier &memoryBarrier) {

        VkImageMemoryBarrier image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image_memory_barrier.oldLayout        = memoryBarrier.oldLayout;
        image_memory_barrier.newLayout        = memoryBarrier.newLayout;
        image_memory_barrier.image            = imageView.getImage().vkImage();
        image_memory_barrier.subresourceRange = imageView.getSubresourceRange();
        image_memory_barrier.srcAccessMask    = memoryBarrier.srcAccessMask;
        image_memory_barrier.dstAccessMask    = memoryBarrier.dstAccessMask;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

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

    void bufferBarrier(const Buffer &buffer, const BufferMemoryBarrier &bufferMemoryBarrier) {
        VkBufferMemoryBarrier memoryBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.srcAccessMask = bufferMemoryBarrier.srcAccessMask;
        memoryBarrier.dstAccessMask = bufferMemoryBarrier.dstAccessMask;
        memoryBarrier.buffer = buffer.vkBuffer();
        memoryBarrier.offset = bufferMemoryBarrier.offset;
        memoryBarrier.size = bufferMemoryBarrier.size;
        vkCmdPipelineBarrier(_commandBuffer,
                             bufferMemoryBarrier.srcStageMask,
                             bufferMemoryBarrier.dstStageMask,
                             0,
                             0, nullptr,
                             1, &memoryBarrier,
                             0, nullptr);
    }

#ifdef AN_DEBUG
    void debugLabelBegin(const char *name, Math::vec4 color) {
        if (vkCmdBeginDebugUtilsLabelEXT) {
            VkDebugUtilsLabelEXT label{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            label.pLabelName = name;
            label.color[0] = color.r;
            label.color[1] = color.g;
            label.color[2] = color.b;
            label.color[3] = color.a;
            vkCmdBeginDebugUtilsLabelEXT(_commandBuffer, &label);
        }
    }

    void debugLabelEnd() {
        if (vkCmdEndDebugUtilsLabelEXT) {
            vkCmdEndDebugUtilsLabelEXT(_commandBuffer);
        }
    }

    void debugLabelInsert(const char *name, Math::vec4 color) {
        if (vkCmdInsertDebugUtilsLabelEXT) {
            VkDebugUtilsLabelEXT label{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            label.pLabelName = name;
            label.color[0] = color.r;
            label.color[1] = color.g;
            label.color[2] = color.b;
            label.color[3] = color.a;
            vkCmdInsertDebugUtilsLabelEXT(_commandBuffer, &label);
        }
    }

#else
    void debugLabelBegin(const char *name, Math::vec4 color) {}
    void debugLabelEnd() {}
    void debugLabelInsert(const char *name, Math::vec4 color) {}
#endif
};

}

#endif//OJOIE_COMMANDENCODER_HPP
