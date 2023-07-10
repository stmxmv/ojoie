//
// Created by Aleudillonam on 9/5/2022.
//

#ifndef OJOIE_HASH_HPP
#define OJOIE_HASH_HPP

#include "ojoie/Render/private/vulkan.hpp"
#include "ojoie/Configuration/typedef.h"
#include <ojoie/Math/Math.hpp>

#include "ojoie/Render/private/vulkan/Image.hpp"
#include "ojoie/Render/private/vulkan/RenderPass.hpp"
#include "ojoie/Render/private/vulkan/DescriptorSetLayout.hpp"

#include "ojoie/Render/RenderPipelineState.hpp"

#include <map>

template <>
struct std::hash<VkDescriptorBufferInfo> {
    size_t operator()(const VkDescriptorBufferInfo &descriptor_buffer_info) const;
};

template <>
struct std::hash<VkDescriptorImageInfo> {
    size_t operator()(const VkDescriptorImageInfo &descriptor_image_info) const;
};

template <>
struct std::hash<VkExtent2D> {
    size_t operator()(const VkExtent2D &extent) const;
};


template <>
struct std::hash<AN::VK::ImageView> {
    std::size_t operator()(const AN::VK::ImageView &imageView) const {
        return std::hash<VkImageView>{}(imageView.vkImageView());
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
struct std::hash<AN::AttachmentDescriptor> {
    std::size_t operator()(const AN::AttachmentDescriptor &attachment) const {
        std::size_t result = 0;
        AN::Math::hash_combine(result, attachment.format);
        AN::Math::hash_combine(result, attachment.samples);
        AN::Math::hash_combine(result, attachment.width);
        AN::Math::hash_combine(result, attachment.height);
        return result;
    }
};

template <>
struct std::hash<AN::LoadStoreInfo> {
    std::size_t operator()(const AN::LoadStoreInfo &load_store_info) const {
        std::size_t result = 0;
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkAttachmentLoadOp>::type>(load_store_info.loadOp));
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkAttachmentStoreOp>::type>(load_store_info.storeOp));
        return result;
    }
};

template<>
struct std::hash<AN::SubpassInfo> {
    std::size_t operator()(const AN::SubpassInfo &subpass_info) const {
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
struct std::hash<AN::ShaderResource> {
    size_t operator()(const AN::ShaderResource &shader_resource) const;
};


//template<>
//struct std::hash<AN::VK::ShaderResource> {
//    size_t operator()(const AN::VK::ShaderResource &shader_resource) const {
//        size_t result = 0;
//
//        if (shader_resource.type == AN::VK::ShaderResourceType::Input ||
//            shader_resource.type == AN::VK::ShaderResourceType::Output ||
//            shader_resource.type == AN::VK::ShaderResourceType::PushConstant ||
//            shader_resource.type == AN::VK::ShaderResourceType::SpecializationConstant) {
//            return result;
//        }
//
//        AN::Math::hash_combine(result, shader_resource.set);
//        AN::Math::hash_combine(result, shader_resource.binding);
//        AN::Math::hash_combine(result, static_cast<std::underlying_type<AN::VK::ShaderResourceType>::type>(shader_resource.type));
//
//        return result;
//    }
//};
//
//template<>
//struct std::hash<AN::VK::ShaderFunction> {
//    size_t operator()(const AN::VK::ShaderFunction &function) const {
//        size_t result = 0;
//        AN::Math::hash_combine(result, std::string_view(function.getEntryPoint()));
//        AN::Math::hash_combine(result, &function.getLibrary());
//        AN::Math::hash_combine(result, function.getStageFlags());
//        return result;
//    }
//};

template<std::ranges::range _Range>
struct std::hash<_Range> {
    size_t operator()(_Range &range) const {
        size_t result = 0;
        for (auto &&item : range) {
            AN::Math::hash_combine(result, item);
        }
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
struct std::hash<AN::RenderPassDescriptor> {
    std::size_t operator()(const AN::RenderPassDescriptor &renderPassDescriptor) const {
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


template<>
struct std::hash<AN::VertexAttributeDescriptor> {
    size_t operator()(const AN::VertexAttributeDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.offset);
        AN::Math::hash_combine(result, descriptor.binding);
        AN::Math::hash_combine(result, descriptor.format);
        AN::Math::hash_combine(result, descriptor.location);
        return result;
    }
};

template<>
struct std::hash<AN::VertexLayoutDescriptor> {
    size_t operator()(const AN::VertexLayoutDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.stepFunction);
        AN::Math::hash_combine(result, descriptor.stride);
        return result;
    }
};

template<>
struct std::hash<AN::VertexDescriptor> {
    size_t operator()(const AN::VertexDescriptor & descriptor) {
        size_t result = 0;
        AN::Math::hash_combine(result, descriptor.attributes);
        AN::Math::hash_combine(result, descriptor.layouts);
        return result;
    }
};

namespace AN {

template <typename T>
inline void hash_param(size_t &seed, T &&value){
    AN::Math::hash_combine(seed, std::forward<T>(value));
}

template<>
inline void hash_param<const char * const &>(size_t &seed, const char *const &value){
    AN::Math::hash_combine(seed, std::string_view(value));
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, T &&first_arg, Args &&... args) {
    hash_param(seed, std::forward<T>(first_arg));
    hash_param(seed, std::forward<Args>(args)...);
}

template <>
inline void hash_param<const std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> &>
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
inline void hash_param<const std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>> &>
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
