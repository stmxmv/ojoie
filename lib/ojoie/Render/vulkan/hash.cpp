//
// Created by aojoie on 4/16/2023.
//

#include "Render/private/vulkan/hash.hpp"

size_t std::hash<VkDescriptorBufferInfo>::operator() (const VkDescriptorBufferInfo &descriptor_buffer_info) const {
    std::size_t result = 0;

    AN::Math::hash_combine(result, descriptor_buffer_info.buffer);
    AN::Math::hash_combine(result, descriptor_buffer_info.range);
    AN::Math::hash_combine(result, descriptor_buffer_info.offset);

    return result;
}

size_t std::hash<VkDescriptorImageInfo>::operator() (const VkDescriptorImageInfo &descriptor_image_info) const {
    std::size_t result = 0;

    AN::Math::hash_combine(result, descriptor_image_info.imageView);
    AN::Math::hash_combine(result, static_cast<std::underlying_type<VkImageLayout>::type>(descriptor_image_info.imageLayout));
    AN::Math::hash_combine(result, descriptor_image_info.sampler);

    return result;
}

size_t std::hash<VkExtent2D>::operator() (const VkExtent2D &extent) const {
    std::size_t result = 0;
    AN::Math::hash_combine(result, extent.width);
    AN::Math::hash_combine(result, extent.height);
    return result;
}

size_t std::hash<AN::ShaderResource>::operator() (const AN::ShaderResource &shader_resource) const {
    size_t result = 0;
    /// this function can only be called when init PipelineLayout or DescriptorSetLayout
    /// so only hash descriptor set types and pushConstant
    if (shader_resource.resourceType == AN::kShaderResourceSpecializationConstant) {
        ANAssert(false && "hash only hash descriptor set types and pushConstant");
        return result;
    }

    AN::Math::hash_combine(result, shader_resource.resourceType);
    AN::Math::hash_combine(result, shader_resource.stages);

//    if (shader_resource.resourceType == AN::kShaderResourcePushConstant) {
        AN::Math::hash_combine(result, shader_resource.block.size);
        AN::Math::hash_combine(result, shader_resource.block.offset);
//        return result;
//    }

    /// same descriptor set layouts are often in the same set
    AN::Math::hash_combine(result, shader_resource.set);
    AN::Math::hash_combine(result, shader_resource.array_size);
    AN::Math::hash_combine(result, shader_resource.binding);
    return result;
}