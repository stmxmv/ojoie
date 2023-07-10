//
// Created by Aleudillonam on 9/6/2022.
//
#include "Render/private/vulkan/RenderPass.hpp"
#include "Render/RenderTarget.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/RenderTypes.hpp"

namespace AN::VK {

RenderPass::RenderPass(RenderPass &&other) noexcept
    : handle(other.handle),
      subpass_count(other.subpass_count), input_attachments(std::move(other.input_attachments)),
      color_attachments(std::move(other.color_attachments)),
      depth_stencil_attachments(std::move(other.depth_stencil_attachments)),
      resolveAttachments(std::move(other.resolveAttachments)),
      _framebuffer(other._framebuffer) {
    other.handle = VK_NULL_HANDLE;
    other._framebuffer = VK_NULL_HANDLE;
}

bool RenderPass::init(const RenderPassDescriptor &renderPassDescriptor) {
    subpass_count = std::max<size_t>(1, renderPassDescriptor.subpasses.size());
    input_attachments.resize(subpass_count);
    color_attachments.resize(subpass_count);
    resolveAttachments.resize(subpass_count);
    depth_stencil_attachments.resize(subpass_count);

    uint32_t depth_stencil_attachment{VK_ATTACHMENT_UNUSED};

    std::vector<VkAttachmentDescription> attachment_descriptions;

    for (uint32_t i = 0U; i < renderPassDescriptor.attachments.size(); ++i) {
        VkAttachmentDescription attachment{};

        attachment.format      = toVkRenderType(renderPassDescriptor.attachments[i].format);
        attachment.samples     = (VkSampleCountFlagBits)renderPassDescriptor.attachments[i].samples;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (i < renderPassDescriptor.loadStoreInfos.size()) {
            attachment.loadOp         = toVkRenderType(renderPassDescriptor.loadStoreInfos[i].loadOp);
            attachment.storeOp        = toVkRenderType(renderPassDescriptor.loadStoreInfos[i].storeOp);
            attachment.stencilLoadOp  = toVkRenderType(renderPassDescriptor.loadStoreInfos[i].loadOp);
            attachment.stencilStoreOp = toVkRenderType(renderPassDescriptor.loadStoreInfos[i].storeOp);
        }

        if (is_depth_stencil_format(attachment.format)) {
            depth_stencil_attachment = i;
            attachment.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        attachment_descriptions.push_back(attachment);
    }

    std::vector<VkSubpassDescription> subpass_descriptions;
    subpass_descriptions.reserve(subpass_count);

    for (size_t i = 0; i < renderPassDescriptor.subpasses.size(); ++i) {
        const auto &subpass = renderPassDescriptor.subpasses[i];

        // Fill color/depth attachments references
        for (auto o_attachment : subpass.colorAttachments) {
            color_attachments[i].push_back({o_attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        }

        // Fill input attachments references
        for (auto i_attachment : subpass.inputAttachments) {
            if (is_depth_stencil_format(attachment_descriptions[i_attachment].format)) {
                input_attachments[i].push_back({i_attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL});
            } else {
                input_attachments[i].push_back({i_attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            }
        }

        if (subpass.depthStencilAttachment >= 0) {
            depth_stencil_attachments[i].push_back({depth_stencil_attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
        }
    }


    for (size_t i = 0; i < renderPassDescriptor.subpasses.size(); ++i) {
        const auto &subpass = renderPassDescriptor.subpasses[i];

        VkSubpassDescription subpass_description{};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass_description.pInputAttachments    = input_attachments[i].empty() ? nullptr : input_attachments[i].data();
        subpass_description.inputAttachmentCount = (uint32_t) (input_attachments[i].size());

        subpass_description.pColorAttachments    = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
        subpass_description.colorAttachmentCount = (uint32_t) (color_attachments[i].size());

        subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].empty() ? nullptr : depth_stencil_attachments[i].data();

        if (subpass.resolveAttachment.has_value()) {
            resolveAttachments[i].attachment        = subpass.resolveAttachment.value();
            resolveAttachments[i].layout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpass_description.pResolveAttachments = &resolveAttachments[i];
        }

        subpass_descriptions.push_back(subpass_description);
    }


    // Default subpass
    if (renderPassDescriptor.subpasses.empty()) {
        VkSubpassDescription subpass_description{};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        for (uint32_t k = 0U; k < attachment_descriptions.size(); ++k) {
            if (k == depth_stencil_attachment) {
                continue;
            }

            color_attachments[0].push_back({k, VK_IMAGE_LAYOUT_GENERAL});
        }

        subpass_description.pColorAttachments = color_attachments[0].data();

        if (depth_stencil_attachment != VK_ATTACHMENT_UNUSED) {
            depth_stencil_attachments[0].push_back({depth_stencil_attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

            subpass_description.pDepthStencilAttachment = depth_stencil_attachments[0].data();
        }

        subpass_descriptions.push_back(subpass_description);
    }


    // Make the initial layout same as in the first subpass using that attachment
    for (auto &subpass : subpass_descriptions) {
        for (uint32_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
            auto reference = subpass.pColorAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        for (uint32_t k = 0U; k < subpass.inputAttachmentCount; ++k) {
            auto reference = subpass.pInputAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        if (subpass.pDepthStencilAttachment) {
            auto reference = *subpass.pDepthStencilAttachment;
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }
    }

    // Make the final layout same as the last subpass layout
    {
        auto &subpass = subpass_descriptions.back();

        for (uint32_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
            const auto &reference = subpass.pColorAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }

        for (uint32_t k = 0U; k < subpass.inputAttachmentCount; ++k) {
            const auto &reference = subpass.pInputAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;

            // Do not use depth attachment if used as input
            if (reference.attachment == depth_stencil_attachment) {
                subpass.pDepthStencilAttachment = nullptr;
            }
        }

        if (subpass.pDepthStencilAttachment) {
            const auto &reference = *subpass.pDepthStencilAttachment;

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }
    }

    // Set subpass dependencies
    std::vector<VkSubpassDependency> dependencies(subpass_count - 1);

    if (subpass_count > 1) {
        for (uint32_t i = 0; i < dependencies.size(); ++i) {
            // Transition input attachments from color attachment to shader read
            dependencies[i].srcSubpass      = i;
            dependencies[i].dstSubpass      = i + 1;
            dependencies[i].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[i].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[i].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[i].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }
    }

    // Create render pass
    VkRenderPassCreateInfo create_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

    create_info.attachmentCount = (uint32_t) (attachment_descriptions.size());
    create_info.pAttachments    = attachment_descriptions.data();
    create_info.subpassCount    = (uint32_t) (subpass_count);
    create_info.pSubpasses      = subpass_descriptions.data();
    create_info.dependencyCount = (uint32_t) (dependencies.size());
    create_info.pDependencies   = dependencies.data();

    auto result = vkCreateRenderPass(GetDevice().vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create RenderPass %s", ResultCString(result));
        return false;
    }

    recreateFramebuffer(renderPassDescriptor.attachments);

    return true;
}

void RenderPass::deinit() {
    // Destroy render pass
    if (handle != VK_NULL_HANDLE) {
        vkDestroyRenderPass(GetDevice().vkDevice(), handle, nullptr);

        vkDestroyFramebuffer(GetDevice().vkDevice(), _framebuffer, nullptr);
        _framebuffer = nullptr;

        handle = VK_NULL_HANDLE;
    }
}

void RenderPass::recreateFramebuffer(std::span<const AttachmentDescriptor> attachmentDescriptors) {
    if (_framebuffer) {
        vkDestroyFramebuffer(GetDevice().vkDevice(), _framebuffer, nullptr);
        _framebuffer = nullptr;
    }

    /// create framebuffers
    auto renderTargetFormats = attachmentDescriptors | std::views::transform([](auto &&renderTarget) {
                                   return toVkRenderType(renderTarget.format);
                               });

    UInt32 framebufferWidth  = std::ranges::min(attachmentDescriptors | std::views::transform([](auto &&extent) {
                                                   return extent.width;
                                               }));
    UInt32 framebufferHeight = std::ranges::min(attachmentDescriptors | std::views::transform([](auto &&extent) {
                                                    return extent.height;
                                                }));


    VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO };
    std::vector<VkFramebufferAttachmentImageInfo> framebufferAttachmentImageInfos;
    framebufferAttachmentImageInfos.reserve(attachmentDescriptors.size());

    std::vector<VkFormat> attachmentFormats(renderTargetFormats.begin(), renderTargetFormats.end());

    int index = 0;
    for (const AttachmentDescriptor &attachmentDescriptor : attachmentDescriptors) {
        VkFramebufferAttachmentImageInfo framebufferAttachmentImageInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO };
        /// currently swapchain and renderTarget image flags is all 0
        framebufferAttachmentImageInfo.flags = RenderTarget::GetImageCreateFlags(attachmentFormats[index]);
        if (attachmentDescriptors[index].format == kRTFormatSwapchain) {
            framebufferAttachmentImageInfo.usage = GetDevice().getSwapchainImageUsageFlags();
        } else {
            framebufferAttachmentImageInfo.usage = RenderTarget::GetImageUsageFlags(attachmentFormats[index]);
        }
        framebufferAttachmentImageInfo.width = attachmentDescriptor.width;
        framebufferAttachmentImageInfo.height = attachmentDescriptor.height;
        framebufferAttachmentImageInfo.viewFormatCount = 1;
        framebufferAttachmentImageInfo.pViewFormats = &attachmentFormats[index];
        framebufferAttachmentImageInfo.layerCount = 1;
        ++index;

        framebufferAttachmentImageInfos.push_back(framebufferAttachmentImageInfo);
    }

    framebufferAttachmentsCreateInfo.attachmentImageInfoCount = framebufferAttachmentImageInfos.size();
    framebufferAttachmentsCreateInfo.pAttachmentImageInfos = framebufferAttachmentImageInfos.data();

    /// we create imageless framebuffer
    VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebufferCreateInfo.renderPass      = handle;
    framebufferCreateInfo.width           = framebufferWidth;
    framebufferCreateInfo.height          = framebufferHeight;
    framebufferCreateInfo.layers          = 1;
    framebufferCreateInfo.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
    framebufferCreateInfo.pNext = &framebufferAttachmentsCreateInfo;

    framebufferCreateInfo.attachmentCount = framebufferAttachmentsCreateInfo.attachmentImageInfoCount;

    VkResult result = vkCreateFramebuffer(GetDevice().vkDevice(), &framebufferCreateInfo, nullptr, &_framebuffer);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Cannot create Framebuffer %s", ResultCString(result));

        /// TODO fatal error abort
    }

}

}// namespace AN::VK