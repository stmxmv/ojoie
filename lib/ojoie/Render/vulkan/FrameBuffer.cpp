//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/FrameBuffer.hpp"

namespace AN::VK {


bool FrameBuffer::init(Device &device, const RenderTarget &render_target, const RenderPass &render_pass) {
    _device = &device;

    const auto &extent = render_target.extent;

    std::vector<VkImageView> attachments;

    for (const auto &view : render_target.views) {
        attachments.emplace_back(view.vkImageView());
    }

    VkFramebufferCreateInfo create_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

    create_info.renderPass      = render_pass.vkRenderPass();
    create_info.attachmentCount = (uint32_t)(attachments.size());
    create_info.pAttachments    = attachments.data();
    create_info.width           = extent.width;
    create_info.height          = extent.height;
    create_info.layers          = 1;

    auto result = vkCreateFramebuffer(_device->vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create Framebuffer %s", ResultCString(result));
        return false;
    }

    return true;
}
void FrameBuffer::deinit() {
    if (handle != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
}