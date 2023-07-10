//
// Created by Aleudillonam on 9/9/2022.
//

#include "Render/private/vulkan/DescriptorSetLayout.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

bool DescriptorSetLayout::init(Device &device, const DescriptorSetLayoutDescriptor &descriptorSetLayoutDescriptor) {
    _device = &device;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.resize(descriptorSetLayoutDescriptor.descriptorSetDescriptors.size());

    const auto &descriptors = descriptorSetLayoutDescriptor.descriptorSetDescriptors;
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        bindings[i].binding = descriptors[i].binding;
        bindings[i].stageFlags = descriptors[i].stageFlags;
        bindings[i].descriptorType = descriptors[i].type;
        bindings[i].descriptorCount = descriptors[i].arraySize;

        bindings_lookup.emplace(bindings[i].binding, bindings[i]);
    }

    VkDescriptorSetLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    create_info.bindingCount = (uint32_t)(bindings.size());
    create_info.pBindings    = bindings.data();

    // Create the Vulkan descriptor set layout handle
    VkResult result = vkCreateDescriptorSetLayout(device.vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create DescriptorSetLayout %s", ResultCString(result));
        return false;
    }

    return true;
}

void DescriptorSetLayout::deinit() {
    if (handle) {
        vkDestroyDescriptorSetLayout(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

}