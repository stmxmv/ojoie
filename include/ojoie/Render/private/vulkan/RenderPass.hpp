//
// Created by Aleudillonam on 9/4/2022.
//

#ifndef OJOIE_VK_RENDERPASS_HPP
#define OJOIE_VK_RENDERPASS_HPP

#include "ojoie/Render/private/vulkan/RenderTarget.hpp"
#include <ojoie/Render/RenderPass.hpp>
#include <ojoie/Template/SmallVector.hpp>

namespace AN::VK {


class RenderPass : public AN::RenderPassImpl, public NonCopyable {

    VkRenderPass handle{VK_NULL_HANDLE};

    size_t subpass_count;

    // Store attachments for every subpass
    SmallVector<SmallVector<VkAttachmentReference>> input_attachments;

    SmallVector<SmallVector<VkAttachmentReference>> color_attachments;

    SmallVector<SmallVector<VkAttachmentReference>> depth_stencil_attachments;

    SmallVector<VkAttachmentReference> resolveAttachments;

    VkFramebuffer _framebuffer{};

public:
    RenderPass() = default;

    RenderPass(RenderPass &&other) noexcept;

    ~RenderPass() {
        deinit();
    }

    virtual bool init(const RenderPassDescriptor &renderPassDescriptor) override;

    virtual void deinit() override;

    VkFramebuffer getFramebuffer() const { return _framebuffer; }

    VkRenderPass vkRenderPass() const { return handle; }

    /// you must recreate the framebuffer if attachment descriptor parameters change
    void recreateFramebuffer(std::span<const AttachmentDescriptor> attachmentDescriptors);

};

}// namespace AN::VK

#endif//OJOIE_VK_RENDERPASS_HPP
