//
// Created by Aleudillonam on 8/28/2022.
//

#ifndef OJOIE_QUEUE_HPP
#define OJOIE_QUEUE_HPP

#include "ojoie/Render/private/vulkan.hpp"



namespace AN::VK {

class Device;
class CommandBuffer;

class Queue : private NonCopyable {
    Device *_device;

    VkQueue handle{};

    uint32_t _familyIndex;

    uint32_t _index;

    VkBool32 _canPresent;

    VkQueueFamilyProperties _properties;


public:

    Queue() = default;

    Queue(Queue &&other) noexcept;

    bool init(Device &device,
              uint32_t family_index,
              const VkQueueFamilyProperties &properties,
              VkBool32 can_present,
              uint32_t index);

    Device &getDevice() const {
        return *_device;
    }

    VkQueue vkQueue() const {
        return handle;
    }

    template<typename VKSubmitInfos>
    VkResult submit(VKSubmitInfos &&submit_infos, VkFence fence) const {
        return vkQueueSubmit(handle, std::size(submit_infos), std::data(submit_infos), fence);
    }

    VkResult submit(VkCommandBuffer command_buffer, VkFence fence) const {
        VkSubmitInfo submit_info[1] = {};
        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers    = &command_buffer;

        return submit(submit_info, fence);
    }

    /// convenient method which call commandBuffer::submit
    void submit(class VK::CommandBuffer &commandBuffer);


    VkResult present(const VkPresentInfoKHR &present_info) const {
        if (!_canPresent) {
            return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;
        }

        return vkQueuePresentKHR(handle, &present_info);
    }

    VkResult waitIdle() const {
        return vkQueueWaitIdle(handle);
    }

    uint32_t getFamilyIndex() const {
        return _familyIndex;
    }

    uint32_t getIndex() const {
        return _index;
    }

    VkBool32 canPresent() const {
        return _canPresent;
    }

    const VkQueueFamilyProperties &getProperties() const {
        return _properties;
    }
};

}

#endif//OJOIE_QUEUE_HPP
