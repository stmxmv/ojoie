//
// Created by Aleudillonam on 9/5/2022.
//

#ifndef OJOIE_HASH_HPP
#define OJOIE_HASH_HPP

#include "Render/private/vulkan.hpp"
#include <ojoie/Core/typedef.h>
#include <ojoie/Math/Math.hpp>

#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan/RenderPass.hpp"
#include "Render/private/vulkan/DescriptorSetLayout.hpp"

#include "Render/RenderPipelineState.hpp"

#include <map>

template <>
struct std::hash<VkDescriptorBufferInfo> {
    size_t operator()(const VkDescriptorBufferInfo &descriptor_buffer_info) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, descriptor_buffer_info.buffer);
        AN::Math::hash_combine(result, descriptor_buffer_info.range);
        AN::Math::hash_combine(result, descriptor_buffer_info.offset);

        return result;
    }
};

template <>
struct std::hash<VkDescriptorImageInfo> {
    size_t operator()(const VkDescriptorImageInfo &descriptor_image_info) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, descriptor_image_info.imageView);
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkImageLayout>::type>(descriptor_image_info.imageLayout));
        AN::Math::hash_combine(result, descriptor_image_info.sampler);

        return result;
    }
};

template <>
struct std::hash<AN::VK::RenderTarget> {
    std::size_t operator()(const AN::VK::RenderTarget &render_target) const {
        std::size_t result = 0;
        for (const auto &view : render_target.views) {
            AN::Math::hash_combine(result, view.vkImageView());
        }

        return result;
    }
};

template <>
struct std::hash<AN::VK::RenderPass> {
    std::size_t operator()(const AN::VK::RenderPass &render_pass) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, render_pass.vkRenderPass());

        return result;
    }
};

template <>
struct std::hash<AN::VK::RenderAttachment> {
    std::size_t operator()(const AN::VK::RenderAttachment &attachment) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkFormat>::type>(attachment.format));
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(attachment.samples));

        return result;
    }
};

template <>
struct std::hash<AN::VK::LoadStoreInfo> {
    std::size_t operator()(const AN::VK::LoadStoreInfo &load_store_info) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkAttachmentLoadOp>::type>(load_store_info.loadOp));
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkAttachmentStoreOp>::type>(load_store_info.storeOp));

        return result;
    }
};

template<>
struct std::hash<AN::VK::SubpassInfo> {
    std::size_t operator()(const AN::VK::SubpassInfo &subpass_info) const {
        std::size_t result = 0;

        for (uint32_t output_attachment : subpass_info.colorAttachments) {
            AN::Math::hash_combine(result, output_attachment);
        }

        for (uint32_t input_attachment : subpass_info.inputAttachments) {
            AN::Math::hash_combine(result, input_attachment);
        }

        return result;
    }
};

template<>
struct std::hash<AN::VK::ShaderResource> {
    size_t operator()(const AN::VK::ShaderResource &shader_resource) const {
        size_t result = 0;

        if (shader_resource.type == AN::VK::ShaderResourceType::Input ||
            shader_resource.type == AN::VK::ShaderResourceType::Output ||
            shader_resource.type == AN::VK::ShaderResourceType::PushConstant ||
            shader_resource.type == AN::VK::ShaderResourceType::SpecializationConstant) {
            return result;
        }

        AN::Math::hash_combine(result, shader_resource.set);
        AN::Math::hash_combine(result, shader_resource.binding);
        AN::Math::hash_combine(result, static_cast<std::underlying_type<AN::VK::ShaderResourceType>::type>(shader_resource.type));

        return result;
    }
};

template<>
struct std::hash<AN::VK::ShaderFunction> {
    size_t operator()(const AN::VK::ShaderFunction &function) const {
        size_t result = 0;
        AN::Math::hash_combine(result, std::string_view(function.getEntryPoint()));
        AN::Math::hash_combine(result, &function.getLibrary());
        AN::Math::hash_combine(result, function.getStageFlags());
        return result;
    }
};

template<typename T>
struct std::hash<std::vector<T>> {
    std::size_t operator()(const std::vector<T> &container) const {
        std::size_t result = 0;
        for (auto &&item : container) {
            AN::Math::hash_combine(result, item);
        }
        return result;
    }
};

template<typename T, size_t N>
struct std::hash<T[N]> {
    std::size_t operator()(const T(&array)[N]) const {
        std::size_t result = 0;
        for (auto &&item : array) {
            AN::Math::hash_combine(result, item);
        }
        return result;
    }
};

template <>
struct std::hash<AN::VK::RenderPassDescriptor> {
    std::size_t operator()(const AN::VK::RenderPassDescriptor &renderPassDescriptor) const {
        std::size_t result = 0;

        for (auto &&item : renderPassDescriptor.attachments) {
            AN::Math::hash_combine(result, item);
        }

        for (auto &&item : renderPassDescriptor.loadStoreInfos) {
            AN::Math::hash_combine(result, item);
        }

        for (auto &&item : renderPassDescriptor.subpasses) {
            AN::Math::hash_combine(result, item);
        }

        return result;
    }
};

template <>
struct std::hash<AN::VK::DescriptorSetLayoutDescriptor> {
    std::size_t operator()(const AN::VK::DescriptorSetLayoutDescriptor &descriptorSetLayoutDescriptor) const {
        std::size_t result = 0;

        for (auto &&item : descriptorSetLayoutDescriptor.descriptorSetDescriptors) {
            AN::Math::hash_combine(result, item.type);
            AN::Math::hash_combine(result, item.stageFlags);
            AN::Math::hash_combine(result, item.arraySize);
            AN::Math::hash_combine(result, item.binding);
        }

        return result;
    }
};

template<typename T, int cap>
struct std::hash<AN::RC::detail::HelperArray<T, cap>> {
    size_t operator()(const AN::RC::detail::HelperArray<T, cap> &array) {
        size_t result = 0;
        for (uint32_t i = 0; i < array.count(); ++i) {
            AN::Math::hash_combine(result, array[i]);
        }
        return result;
    }
};

template<>
struct std::hash<AN::RC::VertexAttributeDescriptor> {
    size_t operator()(const AN::RC::VertexAttributeDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.offset);
        AN::Math::hash_combine(result, descriptor.binding);
        AN::Math::hash_combine(result, descriptor.format);
        AN::Math::hash_combine(result, descriptor.location);
        return result;
    }
};

template<>
struct std::hash<AN::RC::VertexLayoutDescriptor> {
    size_t operator()(const AN::RC::VertexLayoutDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.stepFunction);
        AN::Math::hash_combine(result, descriptor.stride);
        return result;
    }
};

template<>
struct std::hash<AN::RC::RenderPipelineColorAttachmentDescriptor> {
    size_t operator()(const AN::RC::RenderPipelineColorAttachmentDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.blendingEnabled);
        AN::Math::hash_combine(result, descriptor.writeMask);
        AN::Math::hash_combine(result, descriptor.alphaBlendOperation);
        AN::Math::hash_combine(result, descriptor.rgbBlendOperation);
        AN::Math::hash_combine(result, descriptor.sourceAlphaBlendFactor);
        AN::Math::hash_combine(result, descriptor.destinationAlphaBlendFactor);
        AN::Math::hash_combine(result, descriptor.sourceRGBBlendFactor);
        AN::Math::hash_combine(result, descriptor.destinationRGBBlendFactor);
        return result;
    }
};

template<>
struct std::hash<AN::RC::DepthStencilDescriptor> {
    size_t operator()(const AN::RC::DepthStencilDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.depthTestEnabled);
        AN::Math::hash_combine(result, descriptor.depthWriteEnabled);
        AN::Math::hash_combine(result, descriptor.depthCompareFunction);
        /// TODO backFaceStencil, frontFaceStencil
        return result;
    }
};

template<>
struct std::hash<AN::RC::SpecializationConstantState> {
    std::size_t operator()(const AN::RC::SpecializationConstantState &specialization_constant_state) const {
        std::size_t result = 0;

        for (const auto &constants : specialization_constant_state.getSpecializationConstantState()) {
            AN::Math::hash_combine(result, constants.first);
            for (const auto data : constants.second) {
                AN::Math::hash_combine(result, data);
            }
        }

        return result;
    }
};

template<>
struct std::hash<AN::RC::Function> {
    size_t operator()(const AN::RC::Function &function) const {
        size_t result = 0;
        AN::Math::hash_combine(result, std::string_view(function.name));
        AN::Math::hash_combine(result, std::string_view(function.library));
        return result;
    }
};

template <>
struct std::hash<AN::RC::RenderPipelineStateDescriptor> {
    size_t operator()(const AN::RC::RenderPipelineStateDescriptor &renderPipelineDescriptor) const {
        std::size_t result = 0;
        AN::Math::hash_combine(result, renderPipelineDescriptor.vertexFunction);
        AN::Math::hash_combine(result, renderPipelineDescriptor.fragmentFunction);
        AN::Math::hash_combine(result, renderPipelineDescriptor.vertexDescriptor.attributes);
        AN::Math::hash_combine(result, renderPipelineDescriptor.vertexDescriptor.layouts);
        AN::Math::hash_combine(result, renderPipelineDescriptor.colorAttachments);
        AN::Math::hash_combine(result, renderPipelineDescriptor.depthStencilDescriptor);
        AN::Math::hash_combine(result, renderPipelineDescriptor.rasterSampleCount);
        AN::Math::hash_combine(result, renderPipelineDescriptor.alphaToCoverageEnabled);
        AN::Math::hash_combine(result, renderPipelineDescriptor.alphaToOneEnabled);
        AN::Math::hash_combine(result, renderPipelineDescriptor.cullMode);
        AN::Math::hash_combine(result, renderPipelineDescriptor.subpassIndex);
        AN::Math::hash_combine(result, renderPipelineDescriptor.specializationConstantState);

        return result;
    }
};

namespace AN {

template <typename T>
inline void hash_param(size_t &seed, const T &value){
    AN::Math::hash_combine(seed, value);
}

template<>
inline void hash_param<const char *>(size_t &seed, const char *const &value){
    AN::Math::hash_combine(seed, std::string_view(value));
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &... args) {
    hash_param(seed, first_arg);
    hash_param(seed, args...);
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>>>
        (size_t &seed, const std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        for (const auto &bindingElement : binding_set.second) {
            AN::Math::hash_combine(seed, bindingElement.first);
            AN::Math::hash_combine(seed, bindingElement.second);
        }
    }
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>>>
        (size_t &seed, const std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        for (const auto &bindingElement : binding_set.second) {
            AN::Math::hash_combine(seed, bindingElement.first);
            AN::Math::hash_combine(seed, bindingElement.second);
        }
    }
}

}

#endif//OJOIE_HASH_HPP
