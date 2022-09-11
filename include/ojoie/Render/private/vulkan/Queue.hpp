//
// Created by Aleudillonam on 8/28/2022.
//

#ifndef OJOIE_QUEUE_HPP
#define OJOIE_QUEUE_HPP

namespace AN::VK {

class Queue : private NonCopyable {

    VkQueue handle;

    uint32_t _familyIndex;

    uint32_t _index;

    VkBool32 _canPresent;

    VkQueueFamilyProperties _properties;

    VkDevice _device;


public:

    Queue(VkDevice device,
          uint32_t family_index,
          const VkQueueFamilyProperties &properties,
          VkBool32 can_present,
          uint32_t index)
            :_device(device),_familyIndex(family_index), _properties(properties), _canPresent(can_present), _index(index) {

        vkGetDeviceQueue(_device, family_index, index, &handle);
    }

    Queue(Queue &&other) noexcept
        : handle(other.handle), _familyIndex(other._familyIndex), _index(other._index),
          _canPresent(other._canPresent), _properties(other._properties), _device(other._device) {

        memset(&other, 0, sizeof(*this));
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
