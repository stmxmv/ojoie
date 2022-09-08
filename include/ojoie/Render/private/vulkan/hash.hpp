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

        for (uint32_t output_attachment : subpass_info.outputAttachments) {
            AN::Math::hash_combine(result, output_attachment);
        }

        for (uint32_t input_attachment : subpass_info.inputAttachments) {
            AN::Math::hash_combine(result, input_attachment);
        }

        return result;
    }
};

template <template<typename> typename Container, typename T>
struct std::hash<Container<T>> {
    std::size_t operator()(const Container<T> &container) const {
        std::size_t result = 0;
        for (auto &&item : container) {
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

namespace AN {

template <typename T>
inline void hash_param(size_t &seed, const T &value){
    AN::Math::hash_combine(seed, value);
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &... args) {
    hash_param(seed, first_arg);
    hash_param(seed, args...);
}

template <>
inline void hash_param<std::map<uint32_t, VkDescriptorBufferInfo>>
        (size_t &seed, const std::map<uint32_t, VkDescriptorBufferInfo> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        AN::Math::hash_combine(seed, binding_set.second);
    }
}

template <>
inline void hash_param<std::map<uint32_t, VkDescriptorImageInfo>>
        (size_t &seed, const std::map<uint32_t, VkDescriptorImageInfo> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        AN::Math::hash_combine(seed, binding_set.second);
    }
}

}

#endif//OJOIE_HASH_HPP
