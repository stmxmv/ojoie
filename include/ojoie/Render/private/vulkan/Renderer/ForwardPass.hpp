//
// Created by aojoie on 9/25/2022.
//

#ifndef OJOIE_VK_FORWORDPASS_HPP
#define OJOIE_VK_FORWORDPASS_HPP

namespace AN::VK {

class ForwardPass : private NonCopyable {
    Device *_device;
//    mutable VK::RenderPassDescriptor renderPassDescriptor;
    VkClearValue clearValue[2];
    const VK::RenderTarget *currentRenderTarget;
public:

    bool init(Device &device) {
        _device = &device;
//        VK::SubpassInfo subpass_infos[1] = {};
//        VK::LoadStoreInfo load_store[2] = {};

        // Swapchain
        load_store[0].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Depth
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        subpass_infos[0].colorAttachments = { 0 };
        subpass_infos[0].depthStencilAttachment = 1;

        renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));

        clearValue[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        clearValue[1].depthStencil = { 1.0f, 0 };

        return true;
    }

    void deinit() {}

//    VK::RenderTarget createRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
//        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
//        VK::Image depthImage;
//        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
//        imageDescriptor.extent = swapchainImage.getExtent();
//        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
//        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
//        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
//
//        if (!depthImage.init(*_device, imageDescriptor)) {
//            throw AN::Exception("cannot init vulkan depth image");
//        }
//
//        std::vector<VK::Image> images;
//
//        images.emplace_back(std::move(swapchainImage));
//        images.emplace_back(std::move(depthImage));
//
//        std::vector<VK::ImageView> views;
//
////        std::vector<VK::RenderAttachment> renderAttachments;
//
//        for (VK::Image &image : images) {
//            views.emplace_back();
//            if (!views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
//                throw AN::Exception("cannot init vulkan image view");
//            }
//
////            VK::RenderAttachment attachment;
////            attachment.format = image.getFormat();
////            attachment.usage = image.getUsage();
////            attachment.samples = image.getSampleCount();
//
////            renderAttachments.push_back(attachment);
//        }
//
//        return {
//                .extent = extent,
//                .images = std::move(images),
//                .views = std::move(views),
////                .attachments = std::move(renderAttachments),
//        };
//    }

    void beginRenderPass(VK::RenderCommandEncoder &renderCommandEncoder, const VK::RenderTarget &renderTarget) {
        currentRenderTarget = &renderTarget;
//        renderPassDescriptor.attachments = renderTarget.attachments;

        renderCommandEncoder.debugLabelBegin("Forward Pass", { 0.f, 1.f, 0.f, 1.f });

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            // Skip 1 as it is handled later as a depth-stencil attachment

//            renderCommandEncoder.imageBarrier(renderTarget.views[0], memory_barrier);
        }

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

//            renderCommandEncoder.imageBarrier(renderTarget.views[1], memory_barrier);
        }

//        renderCommandEncoder.beginRenderPass(renderTarget.extent, renderTarget.views, renderPassDescriptor, clearValue);
    }

    void endRenderPass(VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.endRenderPass();
        renderCommandEncoder.debugLabelEnd();
        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//            renderCommandEncoder.imageBarrier(currentRenderTarget->views[0], memory_barrier);
        }
    }

};

class ForwardMSAAPass : private NonCopyable {
    Device *_device;
    mutable VK::RenderPassDescriptor renderPassDescriptor;
    VkClearValue clearValue[3];
    VK::RenderTarget *currentRenderTarget;
    uint32_t _msaaSamples;

    uint32_t activeFrameIndex;
#ifdef OJOIE_WITH_EDITOR
    std::vector<VK::RenderTarget> editorViewportRenderTargets;
#endif

public:

    bool init(Device &device, uint32_t msaaSamples) {
        _device = &device;
        _msaaSamples = msaaSamples;
        VK::SubpassInfo subpass_infos[1] = {};
        VK::LoadStoreInfo load_store[3] = {};

        // resolve swapchain image
        load_store[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; /// 0 swapchain image will be resolved anyway
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Depth
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // msaa image
        load_store[2].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        subpass_infos[0].colorAttachments = { 2 };
        subpass_infos[0].resolveAttachment = 0;
        subpass_infos[0].depthStencilAttachment = 1;

        renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));

        clearValue[1].depthStencil = { 1.0f, 0 };
        clearValue[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

        activeFrameIndex = 0;
        return true;
    }

    void deinit() {
#ifdef OJOIE_WITH_EDITOR
        editorViewportRenderTargets.clear();
#endif
    }

    VK::RenderTarget createRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };

        VK::Image msaaImage;
        VK::ImageDescriptor msaaImageDescriptror = VK::ImageDescriptor::Default2D();
        msaaImageDescriptror.extent = swapchainImage.getExtent();
        msaaImageDescriptror.format = swapchainImage.getFormat();
        msaaImageDescriptror.imageUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        msaaImageDescriptror.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        msaaImageDescriptror.sampleCount = (VkSampleCountFlagBits)_msaaSamples;

        if (!msaaImage.init(*_device,msaaImageDescriptror)) {
            throw AN::Exception("cannot init vulkan msaa image");
        }


        VK::Image depthImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.sampleCount = (VkSampleCountFlagBits)_msaaSamples;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!depthImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan depth image");
        }

        std::vector<VK::Image> images;

        images.emplace_back(std::move(swapchainImage));
        images.emplace_back(std::move(depthImage));
        images.emplace_back(std::move(msaaImage));


        std::vector<VK::ImageView> views;

        std::vector<VK::RenderAttachment> renderAttachments;

        for (VK::Image &image : images) {
            views.emplace_back();
            if (!views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
                throw AN::Exception("cannot init vulkan image view");
            }

            VK::RenderAttachment attachment;
            attachment.format = image.getFormat();
            attachment.usage = image.getUsage();
            attachment.samples = image.getSampleCount();

            renderAttachments.push_back(attachment);
        }

#ifdef OJOIE_WITH_EDITOR
        imageDescriptor.format = swapchainImage.getFormat();
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageDescriptor.sampleCount = VK_SAMPLE_COUNT_1_BIT;

        VK::Image editorViewportImage;
        if (!editorViewportImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("fail to init vulkan editorViewportImage");
        }

        if (editorViewportRenderTargets.size() < index + 1) {
            editorViewportRenderTargets.resize(index + 1);
        }
        editorViewportRenderTargets[index].clear();
        editorViewportRenderTargets[index].extent = extent;
        editorViewportRenderTargets[index].images.emplace_back(std::move(editorViewportImage));

        if (!editorViewportRenderTargets[index].generateViewsAndAttachments()) {
            throw AN::Exception("could not init vulkan image view for editor viewport image");
        }

#endif

        return {
                .extent = extent,
                .images = std::move(images),
                .views = std::move(views),
                .attachments = std::move(renderAttachments),
        };
    }

    void beginRenderPass(VK::RenderCommandEncoder &renderCommandEncoder, VK::RenderTarget &renderTarget, uint32_t index) {
        currentRenderTarget = &renderTarget;
        renderPassDescriptor.attachments = renderTarget.attachments;
        activeFrameIndex = index;

        renderCommandEncoder.debugLabelBegin("Forward MSAA Pass", { 0.f, 1.f, 0.f, 1.f });

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            // Skip 1 as it is handled later as a depth-stencil attachment

            renderCommandEncoder.imageBarrier(renderTarget.views[0], memory_barrier);
            renderCommandEncoder.imageBarrier(renderTarget.views[2], memory_barrier);
        }

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            renderCommandEncoder.imageBarrier(renderTarget.views[1], memory_barrier);
        }

#ifdef OJOIE_WITH_EDITOR
        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            renderCommandEncoder.imageBarrier(editorViewportRenderTargets[activeFrameIndex].views[0], memory_barrier);
        }

        renderPassDescriptor.attachments[0] = editorViewportRenderTargets[activeFrameIndex].attachments[0];

        std::vector<ImageView> views(3);
        std::move(renderTarget.views.begin() + 1, renderTarget.views.end(), views.begin() + 1);
        views[0] = std::move(editorViewportRenderTargets[activeFrameIndex].views[0]);

        VK::SubpassInfo subpass_infos[1] = {};
        subpass_infos[0].colorAttachments = { 2 };
        subpass_infos[0].resolveAttachment = 0;
        subpass_infos[0].depthStencilAttachment = 1;

        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));


        renderCommandEncoder.beginRenderPass(editorViewportRenderTargets[activeFrameIndex].extent,
                                             views,
                                             renderPassDescriptor,
                                             clearValue);

        std::move(views.begin() + 1, views.end(), renderTarget.views.begin() + 1);
        editorViewportRenderTargets[activeFrameIndex].views[0] = std::move(views[0]);
#else

        renderCommandEncoder.beginRenderPass(renderTarget.extent, renderTarget.views, renderPassDescriptor, clearValue);
#endif


    }

    void endRenderPass(VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.endRenderPass();
        renderCommandEncoder.debugLabelEnd();
        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            renderCommandEncoder.imageBarrier(currentRenderTarget->views[0], memory_barrier);
        }
    }


#ifdef OJOIE_WITH_EDITOR

    ImageView &getFrameEditorViewportImageView() {
        return editorViewportRenderTargets[activeFrameIndex].views[0];
    }

    void nextEditorPass(const RenderContext &renderContext,
                        VK::RenderCommandEncoder &renderCommandEncoder) {

        renderCommandEncoder.endRenderPass();
        renderCommandEncoder.debugLabelEnd();
        renderCommandEncoder.debugLabelEnd();

        renderCommandEncoder.debugLabelBegin("Editor Pass", { 0.4f, 0.76f, 0.1f, 1.f });

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            renderCommandEncoder.imageBarrier(editorViewportRenderTargets[activeFrameIndex].views[0], memory_barrier);
        }

        VK::SubpassInfo subpass_infos[1] = {};
        subpass_infos[0].colorAttachments = { 0 };

        renderPassDescriptor.attachments.resize(1);
        renderPassDescriptor.attachments[0] = currentRenderTarget->attachments[0];
        renderPassDescriptor.subpasses.assign(subpass_infos, subpass_infos + 1);

        std::vector<ImageView> views(1);
        std::move(currentRenderTarget->views.begin(), currentRenderTarget->views.begin() + 1, views.begin());

        VkClearValue clearValue[1];
        clearValue[0].color = { 0.f, 0.f, 0.f, 1.f }; // don't actually need this
        renderCommandEncoder.beginRenderPass(currentRenderTarget->extent,
                                             views,
                                             renderPassDescriptor,
                                             clearValue);

        std::move(views.begin(), views.end(), currentRenderTarget->views.begin());
    }

#endif

};


}

#endif//OJOIE_VK_FORWORDPASS_HPP
