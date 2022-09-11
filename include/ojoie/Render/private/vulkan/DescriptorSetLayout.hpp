//
// Created by Aleudillonam on 9/9/2022.
//

#ifndef OJOIE_DESCRIPTORSETLAYOUT_HPP
#define OJOIE_DESCRIPTORSETLAYOUT_HPP

#include "Render/private/vulkan.hpp"

namespace AN::VK {

class Device;

struct DescriptorSetDescriptor {
    VkShaderStageFlags stageFlags;
    VkDescriptorType type;
    uint32_t binding;
    uint32_t arraySize;
};

struct DescriptorSetLayoutDescriptor {
    std::vector<DescriptorSetDescriptor> descriptorSetDescriptors;
};

class DescriptorSetLayout : private NonCopyable {

    Device *_device;

    VkDescriptorSetLayout handle{VK_NULL_HANDLE};

public:

    DescriptorSetLayout() = default;

    DescriptorSetLayout(DescriptorSetLayout &&other) noexcept : _device(other._device), handle(other.handle) {
        other.handle = VK_NULL_HANDLE;
    }

    ~DescriptorSetLayout() {
        deinit();
    }

    bool init(Device &device, const DescriptorSetLayoutDescriptor &descriptorSetLayoutDescriptor);

    void deinit();

    VkDescriptorSetLayout vkDescriptorSetLayout() const {
        return handle;
    }

};

}

#endif//OJOIE_DESCRIPTORSETLAYOUT_HPP
