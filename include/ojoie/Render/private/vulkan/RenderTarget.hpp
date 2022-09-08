//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_RENDERTARGET_HPP
#define OJOIE_RENDERTARGET_HPP

#include "Render/private/vulkan/Image.hpp"

namespace AN::VK {

struct RenderAttachment {
    typedef RenderAttachment Self;

    VkFormat format;

    VkSampleCountFlagBits samples;

    VkImageUsageFlags usage;

    static Self Default() {
        return {
                .format = VK_FORMAT_UNDEFINED,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        };
    }
};

struct RenderTarget {
    Device *_device;

    VkExtent2D extent;

    std::vector<Image> images;

    std::vector<ImageView> views;

    std::vector<RenderAttachment> attachments;

    /// By default there are no input attachments
    std::vector<uint32_t> input_attachments = {};

    /// By default the output attachments is attachment 0
    std::vector<uint32_t> output_attachments = {0};


    void clear() {
        images.clear();
        views.clear();
    }

};


}

#endif//OJOIE_RENDERTARGET_HPP
