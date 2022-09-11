//
// Created by Aleudillonam on 9/4/2022.
//

#ifndef OJOIE_RENDERPASS_HPP
#define OJOIE_RENDERPASS_HPP

#include "Render/private/vulkan/RenderTarget.hpp"

namespace AN::VK {

struct LoadStoreInfo {
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;

    static LoadStoreInfo Default() {
        return {
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };
    }
};

struct SubpassInfo {
    std::vector<uint32_t> inputAttachments;
    std::vector<uint32_t> outputAttachments;
    uint32_t resolveAttachment = -1;
};

struct RenderPassDescriptor {
    std::vector<RenderAttachment> attachments;
    std::vector<LoadStoreInfo> loadStoreInfos;
    std::vector<SubpassInfo> subpasses;
};

class RenderPass {
    Device *_device;

    VkRenderPass handle{VK_NULL_HANDLE};

    size_t subpass_count;

    // Store attachments for every subpass
    std::vector<std::vector<VkAttachmentReference>> input_attachments;

    std::vector<std::vector<VkAttachmentReference>> color_attachments;

    std::vector<std::vector<VkAttachmentReference>> depth_stencil_attachments;

    std::vector<VkAttachmentReference> resolveAttachments;

public:

    RenderPass() = default;

    RenderPass(RenderPass &&other) noexcept
        : _device(other._device), handle(other.handle),
          subpass_count(other.subpass_count), input_attachments(std::move(other.input_attachments)),
          color_attachments(std::move(other.color_attachments)),
          depth_stencil_attachments(std::move(other.depth_stencil_attachments)) {

        other.handle = VK_NULL_HANDLE;
    }

    ~RenderPass() {
        deinit();
    }

    bool init(Device &device, const RenderPassDescriptor &renderPassDescriptor);


    void deinit();

    VkRenderPass vkRenderPass() const {
        return handle;
    }
};

}

#endif//OJOIE_RENDERPASS_HPP
