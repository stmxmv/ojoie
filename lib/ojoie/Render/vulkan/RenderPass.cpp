//
// Created by Aleudillonam on 9/6/2022.
//
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/RenderPass.hpp"

namespace AN::VK {


bool RenderPass::init(Device &device, const RenderPassDescriptor &renderPassDescriptor) {
    _device = &device;
    subpass_count = std::max<size_t>(1, renderPassDescriptor.subpasses.size());
    input_attachments.resize(subpass_count);
    color_attachments.resize(subpass_count);
    resolveAttachments.resize(subpass_count);
    depth_stencil_attachments.resize(subpass_count);

    uint32_t depth_stencil_attachment{VK_ATTACHMENT_UNUSED};

    std::vector<VkAttachmentDescription> attachment_descriptions;

    for (uint32_t i = 0U; i < renderPassDescriptor.attachments.size(); ++i) {
        VkAttachmentDescription attachment{};

        attachment.format      = renderPassDescriptor.attachments[i].format;
        attachment.samples     = renderPassDescriptor.attachments[i].samples;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (i < renderPassDescriptor.loadStoreInfos.size()) {
            attachment.loadOp         = renderPassDescriptor.loadStoreInfos[i].loadOp;
            attachment.storeOp        = renderPassDescriptor.loadStoreInfos[i].storeOp;
            attachment.stencilLoadOp  = renderPassDescriptor.loadStoreInfos[i].loadOp;
            attachment.stencilStoreOp = renderPassDescriptor.loadStoreInfos[i].storeOp;
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
        subpass_description.inputAttachmentCount = (uint32_t)(input_attachments[i].size());

        subpass_description.pColorAttachments    = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
        subpass_description.colorAttachmentCount = (uint32_t)(color_attachments[i].size());

        subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].empty() ? nullptr : depth_stencil_attachments[i].data();

        if (subpass.resolveAttachment >= 0) {
            resolveAttachments[i].attachment = subpass.resolveAttachment;
            resolveAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

    create_info.attachmentCount = (uint32_t)(attachment_descriptions.size());
    create_info.pAttachments    = attachment_descriptions.data();
    create_info.subpassCount    = (uint32_t)(subpass_count);
    create_info.pSubpasses      = subpass_descriptions.data();
    create_info.dependencyCount = (uint32_t)(dependencies.size());
    create_info.pDependencies   = dependencies.data();

    auto result = vkCreateRenderPass(_device->vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create RenderPass %s", ResultCString(result));
        return false;
    }


    return true;
}
void RenderPass::deinit() {
    // Destroy render pass
    if (handle != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
}