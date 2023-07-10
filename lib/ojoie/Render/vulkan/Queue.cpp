//
// Created by Aleudillonam on 9/12/2022.
//

#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/Device.hpp"
#include <ojoie/Render/private/vulkan/CommandBuffer.hpp>

namespace AN::VK {


bool Queue::init(Device &device,
                 uint32_t family_index,
                 const VkQueueFamilyProperties &properties,
                 VkBool32 can_present,
                 uint32_t index) {

    _device = &device;
    _familyIndex = family_index;
    _properties = properties;
    _canPresent = can_present;
    _index = index;

    vkGetDeviceQueue(_device->vkDevice(), family_index, index, &handle);

    return handle != nullptr;
}

void Queue::submit(CommandBuffer &commandBuffer) {
    commandBuffer.submit(*this);
}

Queue::Queue(Queue &&other) noexcept
        : handle(other.handle), _familyIndex(other._familyIndex), _index(other._index),
          _canPresent(other._canPresent), _properties(other._properties), _device(other._device) {

    memset(&other, 0, sizeof(*this));
}

}