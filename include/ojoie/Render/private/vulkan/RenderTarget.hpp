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
    VkExtent2D extent;

    std::vector<Image> images;

    std::vector<ImageView> views;

    std::vector<RenderAttachment> attachments;

    bool generateViewsAndAttachments() {
        for (VK::Image &image : images) {
            views.emplace_back();
            if (!views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
                return false;
            }

            attachments.push_back({ .format = image.getFormat(),
                                    .samples = image.getSampleCount(),
                                    .usage = image.getUsage() });
        }
        return true;
    }

    void clear() {
        views.clear(); // deinit imageViews before images
        images.clear();
        attachments.clear();
    }

};


}

#endif//OJOIE_RENDERTARGET_HPP
