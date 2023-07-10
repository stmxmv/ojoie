//
// Created by Aleudillonam on 9/4/2022.
//

#ifndef OJOIE_FRAMEBUFFER_HPP
#define OJOIE_FRAMEBUFFER_HPP

#include "ojoie/Render/private/vulkan/RenderPass.hpp"

namespace AN::VK {

class FrameBuffer {

    Device *_device;
    VkFramebuffer handle{VK_NULL_HANDLE};
public:

    FrameBuffer() = default;

    FrameBuffer(FrameBuffer &&other) noexcept : _device(other._device), handle(other.handle) {
        other.handle = VK_NULL_HANDLE;
    }

    ~FrameBuffer() {
        deinit();
    }

    template<typename ImageViews>
    bool init(Device &device, const RenderPass &render_pass, VkExtent2D extent, ImageViews &&attachments) {
        return init(device, render_pass, extent, std::data(attachments), std::size(attachments));
    }

    bool init(Device &device, const RenderPass &render_pass, VkExtent2D extent, const ImageView *attachments, uint32_t size);

    void deinit();

    VkFramebuffer vkFramebuffer() const {
        return handle;
    }


};


}

#endif//OJOIE_FRAMEBUFFER_HPP
