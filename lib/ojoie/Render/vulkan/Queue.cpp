//
// Created by Aleudillonam on 9/12/2022.
//

#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/Device.hpp"


namespace AN::VK {


Queue::Queue(Device &device, uint32_t family_index, const VkQueueFamilyProperties &properties, VkBool32 can_present, uint32_t index)
        :_device(&device),_familyIndex(family_index), _properties(properties), _canPresent(can_present), _index(index) {

    vkGetDeviceQueue(_device->vkDevice(), family_index, index, &handle);
}

}