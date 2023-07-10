//
// Created by Aleudillonam on 9/9/2022.
//

#ifndef OJOIE_DESCRIPTORSETLAYOUT_HPP
#define OJOIE_DESCRIPTORSETLAYOUT_HPP

#include "ojoie/Render/private/vulkan.hpp"
#include <ojoie/Render/private/vulkan/RenderTypes.hpp>
#include <optional>

namespace AN::VK {

class Device;

struct DescriptorSetBindingDescriptor {
    VkShaderStageFlags stageFlags;
    VkDescriptorType type;
    uint32_t binding;
    uint32_t arraySize;
};

struct DescriptorSetLayoutDescriptor {
    std::vector<DescriptorSetBindingDescriptor> descriptorSetDescriptors;
};

namespace detail {
inline VkDescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic) {
    switch (resource_type) {
        case kShaderResourceInputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case kShaderResourceImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case kShaderResourceImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case kShaderResourceImageStorage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case kShaderResourceSampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case kShaderResourceBufferUniform:
            if (dynamic) {
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            } else {
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
        case kShaderResourceBufferStorage:
            if (dynamic) {
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            } else {
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            }
        default:
            throw std::runtime_error("No conversion possible for the shader resource type.");
    }
}
}
class DescriptorSetLayout : private NonCopyable {

    Device *_device;

    VkDescriptorSetLayout handle{VK_NULL_HANDLE};

    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_lookup;

public:

    DescriptorSetLayout() = default;

    DescriptorSetLayout(DescriptorSetLayout &&other) noexcept
        : _device(other._device), handle(other.handle), bindings_lookup(std::move(other.bindings_lookup)) {
        other.handle = VK_NULL_HANDLE;
    }

    ~DescriptorSetLayout() {
        deinit();
    }

    bool init(Device &device, const DescriptorSetLayoutDescriptor &descriptorSetLayoutDescriptor);

    template<typename ShaderResources>
    bool init(Device &device, ShaderResources &&resources, bool dynamicResources) {
        DescriptorSetLayoutDescriptor descriptor{};

        for (const ShaderResource &resource : resources) {
            // Skip shader resources whitout a binding point
            if (resource.resourceType == kShaderResourcePushConstant ||
                resource.resourceType == kShaderResourceSpecializationConstant) {
                continue;
            }

            // Convert from ShaderResourceType to VkDescriptorType.
            auto descriptor_type = detail::find_descriptor_type(resource.resourceType, dynamicResources);

            // Convert ShaderResource to VkDescriptorSetLayoutBinding
            DescriptorSetBindingDescriptor layout_binding{};

            layout_binding.binding    = resource.binding;
            layout_binding.arraySize  = resource.array_size;
            layout_binding.type       = descriptor_type;
            layout_binding.stageFlags = to_VkRenderType((ShaderStageFlagBits)resource.stages);

            descriptor.descriptorSetDescriptors.push_back(layout_binding);
        }

        return init(device, descriptor);
    }

    bool hasLayoutBinding(uint32_t binding) const {
        return bindings_lookup.contains(binding);
    }

    std::optional<VkDescriptorSetLayoutBinding> getLayoutBinding(uint32_t binding) const {
        auto it = bindings_lookup.find(binding);
        if (it == bindings_lookup.end()) {
            return {};
        }
        return it->second;
    }

    void deinit();

    VkDescriptorSetLayout vkDescriptorSetLayout() const {
        return handle;
    }

};

}

#endif//OJOIE_DESCRIPTORSETLAYOUT_HPP
