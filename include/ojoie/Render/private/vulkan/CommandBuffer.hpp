//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_VK_COMMANDBUFFER_HPP
#define OJOIE_VK_COMMANDBUFFER_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/BlitCommandEncoder.hpp"

namespace AN::VK {

class Queue;
class CommandPool;
class FencePool;
class SemaphorePool;

struct Presentable {
    VkSemaphore signalSemaphore, acquireSemaphore;
    VkPipelineStageFlags waitStageFlag;
    VkSwapchainKHR swapchain;
    uint32_t imageIndex;
};

class CommandBuffer {

    const Queue *_queue;
    VkCommandBuffer _commandBuffer;
    CommandBufferResetMode _resetMode;
    VkFence _fence;

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageFlags;

    std::vector<VkSemaphore> signalSemaphores;

    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t> imageIndices;
public:

    CommandBuffer() = default;

    CommandBuffer(const Queue &queue, VkCommandBuffer commandBuffer, CommandBufferResetMode resetMode, VkFence fence)
        : _queue(&queue), _commandBuffer(commandBuffer), _resetMode(resetMode), _fence(fence) {
        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional
        vkBeginCommandBuffer(_commandBuffer, &beginInfo);
    }

    void reset() {
        if (_resetMode == CommandBufferResetMode::ResetIndividually) {
            vkResetFences(_queue->getDevice().vkDevice(), 1, &_fence);
            vkResetCommandBuffer(_commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

            VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginInfo.pInheritanceInfo = nullptr; // Optional
            vkBeginCommandBuffer(_commandBuffer, &beginInfo);

        } else {
            throw AN::Exception("CommandBuffer with reset mode that is not CommandBufferResetMode::ResetIndividually cannot reset");
        }
    }

    BlitCommandEncoder blitCommandEncoder() {
        return BlitCommandEncoder{ _commandBuffer };
    }

    /// \brief register the presentable to present as soon as possiable
    void presentDrawable(const Presentable &presentable) {
        swapchains.push_back(presentable.swapchain);
        imageIndices.push_back(presentable.imageIndex);
        waitSemaphores.push_back(presentable.acquireSemaphore); // wait until next image acquired
        waitDstStageFlags.push_back(presentable.waitStageFlag);
        signalSemaphores.push_back(presentable.signalSemaphore);
    }

    /// \brief submit to the associated queue, cannot be called concurrently, after submitting, the only operation is waiting
    /// \attention submit multiple times is not yet supported
    void submit() {
        if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
            ANLog("failed to end recording command buffer!");
        }

        VkSubmitInfo submit_info[1] = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submit_info[0].commandBufferCount   = 1;
        submit_info[0].pCommandBuffers      = &_commandBuffer;

        submit_info[0].waitSemaphoreCount   = waitSemaphores.size();
        submit_info[0].pWaitSemaphores      = waitSemaphores.data();
        submit_info[0].pWaitDstStageMask    = waitDstStageFlags.data();

        submit_info[0].signalSemaphoreCount = signalSemaphores.size();
        submit_info[0].pSignalSemaphores    = signalSemaphores.data();

        _queue->submit(submit_info, _fence);

        if (!swapchains.empty()) {
            VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            present_info.waitSemaphoreCount = signalSemaphores.size();
            present_info.pWaitSemaphores    = signalSemaphores.data(); // wait until render command complete
            present_info.swapchainCount     = swapchains.size();
            present_info.pSwapchains        = swapchains.data();
            present_info.pImageIndices      = imageIndices.data();
            _queue->present(present_info); /// we just ignore the present result, since acquire next image will return the same result
        }
    }

    /// \brief wait single command buffer to complete
    void waitUntilCompleted() {
        vkWaitForFences(_queue->getDevice().vkDevice(), 1, &_fence, VK_TRUE, UINT_MAX);
    }

    VkCommandBuffer vkCommandBuffer() const {
        return _commandBuffer;
    }

    const Queue &getQueue() const {
        return *_queue;
    }
};


}

#endif//OJOIE_VK_COMMANDBUFFER_HPP
