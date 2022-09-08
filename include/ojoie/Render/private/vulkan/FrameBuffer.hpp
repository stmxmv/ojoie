//
// Created by Aleudillonam on 9/4/2022.
//

#ifndef OJOIE_FRAMEBUFFER_HPP
#define OJOIE_FRAMEBUFFER_HPP

#include "Render/private/vulkan/RenderPass.hpp"

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

    bool init(Device &device, const RenderTarget &render_target, const RenderPass &render_pass);

    void deinit();

    VkFramebuffer vkFramebuffer() const {
        return handle;
    }


};


}

#endif//OJOIE_FRAMEBUFFER_HPP
