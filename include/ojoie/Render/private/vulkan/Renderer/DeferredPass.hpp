//
// Created by aojoie on 9/25/2022.
//

#ifndef OJOIE_VK_DEFERREDPASS_HPP
#define OJOIE_VK_DEFERREDPASS_HPP

namespace AN::VK {


namespace detail {

bool initLightingPipelineState(VK::RenderPipelineState &pipelineState) {
    RC::VertexDescriptor vertexDescriptor{};

    RC::DepthStencilDescriptor depthStencilDescriptor{};
    depthStencilDescriptor.depthTestEnabled = true;
    depthStencilDescriptor.depthWriteEnabled = true;
    depthStencilDescriptor.depthCompareFunction = RC::CompareFunction::Less;

    RC::RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
    renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "lighting.vert.spv" };
    renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "lighting.frag.spv" };

    renderPipelineStateDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
    renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = false;

    renderPipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
    renderPipelineStateDescriptor.depthStencilDescriptor = depthStencilDescriptor;

    renderPipelineStateDescriptor.rasterSampleCount = 1;
    renderPipelineStateDescriptor.alphaToOneEnabled = false;
    renderPipelineStateDescriptor.alphaToCoverageEnabled = false;

    renderPipelineStateDescriptor.cullMode = RC::CullMode::None;

    return pipelineState.init(renderPipelineStateDescriptor);
}

bool initTAAPipelineState(VK::RenderPipelineState &pipelineState) {
    RC::VertexDescriptor vertexDescriptor{};
    
    RC::DepthStencilDescriptor depthStencilDescriptor{};
    depthStencilDescriptor.depthTestEnabled = true;
    depthStencilDescriptor.depthWriteEnabled = true;
    depthStencilDescriptor.depthCompareFunction = RC::CompareFunction::Less;

    RC::RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
    renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "TAA.vert.spv" };
    renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "TAA.frag.spv" };

    renderPipelineStateDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
    renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = false;

    renderPipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
    renderPipelineStateDescriptor.depthStencilDescriptor = depthStencilDescriptor;

    renderPipelineStateDescriptor.rasterSampleCount = 1;
    renderPipelineStateDescriptor.alphaToOneEnabled = false;
    renderPipelineStateDescriptor.alphaToCoverageEnabled = false;

    renderPipelineStateDescriptor.cullMode = RC::CullMode::None;

    return pipelineState.init(renderPipelineStateDescriptor);
}

}

class DeferredPass : private NonCopyable {
    Device *_device;
    mutable VK::RenderPassDescriptor renderPassDescriptor;
    VkClearValue clearValue[4];
    const VK::RenderTarget *currentRenderTarget;

    VK::RenderPipelineState lightingPipelineState;

public:
    bool init(Device &device) {
        _device = &device;
        if (!detail::initLightingPipelineState(lightingPipelineState)) {
            return false;
        }

        VK::SubpassInfo subpass_infos[2] = {};
        VK::LoadStoreInfo load_store[4] = {};

        // resolve swapchain image
        load_store[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; /// 0 swapchain image will be replaced anyway
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Depth
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // albedo
        load_store[2].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // normal
        load_store[3].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        subpass_infos[0].colorAttachments = { 2, 3 };
        subpass_infos[0].depthStencilAttachment = 1;

        subpass_infos[1].inputAttachments = { 1, 2, 3 };
        subpass_infos[1].colorAttachments = { 0 };

        renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));
        
        clearValue[1].depthStencil = { 1.0f, 0 };
        clearValue[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        clearValue[3].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

        return true;
    }

    void deinit() {
        lightingPipelineState.deinit();
    }

    VK::RenderTarget createRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
        VK::Image albedoImage, normalImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!albedoImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan albedo image");
        }

        imageDescriptor.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        if (!normalImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan normal image");
        }

        VK::Image depthImage;
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!depthImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan depth image");
        }

        std::vector<VK::Image> images;

        images.emplace_back(std::move(swapchainImage));
        images.emplace_back(std::move(depthImage));
        images.emplace_back(std::move(albedoImage));
        images.emplace_back(std::move(normalImage));

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

        return {
                ._device = _device,
                .extent = extent,
                .images = std::move(images),
                .views = std::move(views),
                .attachments = std::move(renderAttachments),
                .input_attachments = {},
                .output_attachments = { 0 }
        };

    }

    
    void beginGeometryPass(VK::RenderCommandEncoder &renderCommandEncoder, const VK::RenderTarget &renderTarget) {
        currentRenderTarget = &renderTarget;
        renderPassDescriptor.attachments = renderTarget.attachments;
        
        renderCommandEncoder.debugLabelBegin("Deferred Pass", { 0.f, 1.f, 0.f, 1.f });
        renderCommandEncoder.debugLabelBegin("Geometry Pass", { 0.f, 0.5f, 0.4f, 1.f });

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
            renderCommandEncoder.imageBarrier(renderTarget.views[3], memory_barrier);
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

        renderCommandEncoder.beginRenderPass(renderTarget.extent, renderTarget.views, renderPassDescriptor, clearValue);
    }
    
    void nextLightingPass(const RenderContext &renderContext, VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.debugLabelEnd();
        renderCommandEncoder.debugLabelBegin("Lighting Pass", { 0.f, 1.f, 1.f, 1.f });
        renderCommandEncoder.nextSubPass();
        
        auto cameraNode = Node3D::GetCurrentCamera();
        if (cameraNode) {
            renderCommandEncoder.setRenderPipelineState(lightingPipelineState);
            struct LightingGlobalUniform {
                alignas(16) Math::mat4 inv_view_proj;
                alignas(8)  Math::vec2 inv_resolution;
            };
            LightingGlobalUniform light_uniform{};
            // Inverse resolution
            light_uniform.inv_resolution.x = 1.0f / renderContext.frameWidth;
            light_uniform.inv_resolution.y = 1.0f / renderContext.frameHeight;
            // Inverse view projection

            light_uniform.inv_view_proj = Math::inverse(cameraNode->getProjectionMatrix() * cameraNode->getViewMatrix());

            renderCommandEncoder.pushConstants(0, sizeof light_uniform, &light_uniform);

            renderCommandEncoder.bindImageView(0, currentRenderTarget->views[1]);
            renderCommandEncoder.bindImageView(2, currentRenderTarget->views[2]);
            renderCommandEncoder.bindImageView(3, currentRenderTarget->views[3]);

            renderCommandEncoder.draw(6);
        }
    }

    void endRenderPass(VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.endRenderPass();
        renderCommandEncoder.debugLabelEnd();
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

};

class DeferredTAAPass : private NonCopyable {
    Device *_device;
    mutable VK::RenderPassDescriptor deferredRenderPassDescriptor;
    mutable VK::RenderPassDescriptor taaRenderPassDescriptor;
    VkClearValue deferredClearValue[5];
    
    const VK::RenderTarget *currentRenderTarget;
    
    VK::RenderPipelineState taaPipelineState;
    std::vector<VK::RenderTarget> deferredRenderTargets;
    
    VK::RenderPipelineState lightingPipelineState;
    RC::Sampler taaSampler;
    
    uint32_t activeFrameIndex;
    uint32_t layerFrameCount;
public:

    bool init(Device &device) {
        _device = &device;
        if (!detail::initLightingPipelineState(lightingPipelineState)) {
            return false;
        }
        if (!detail::initTAAPipelineState(taaPipelineState)) {
            return false;
        }
        
        layerFrameCount = 0;
        activeFrameIndex = 0;
        
        RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();
        samplerDescriptor.addressModeU = RC::SamplerAddressMode::ClampToEdge;
        samplerDescriptor.addressModeV = RC::SamplerAddressMode::ClampToEdge;
        samplerDescriptor.addressModeW = RC::SamplerAddressMode::ClampToEdge;
        
        if (!taaSampler.init(samplerDescriptor)) {
            return false;
        }
        
        /// deferred shading
        VK::SubpassInfo subpass_infos[2] = {};
        VK::LoadStoreInfo load_store[5] = {};
        
        // Depth
        load_store[0].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // albedo
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // normal
        load_store[2].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // velocity
        load_store[3].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // current color
        load_store[4].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        subpass_infos[0].colorAttachments = { 1, 2, 3 };
        subpass_infos[0].depthStencilAttachment = 1;

        subpass_infos[1].inputAttachments = { 0, 1, 2 };
        subpass_infos[1].colorAttachments = { 4 };

        deferredRenderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
        deferredRenderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));
        
        deferredClearValue[0].depthStencil = { 1.0f, 0 };
        deferredClearValue[1].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        deferredClearValue[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        deferredClearValue[3].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        deferredClearValue[4].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        
        
        VK::SubpassInfo subpassInfo{};
        subpassInfo.colorAttachments = { 0 };
        
        VK::LoadStoreInfo loadStoreInfo{};
        loadStoreInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we draw the whole screen
        loadStoreInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        taaRenderPassDescriptor.subpasses = { subpassInfo };
        taaRenderPassDescriptor.loadStoreInfos = { loadStoreInfo };
        
        return true;
    }
    
    void deinit() {
        lightingPipelineState.deinit();
        taaSampler.deinit();
        taaPipelineState.deinit();
        deferredRenderTargets.clear();
    }
 
    VK::RenderTarget createRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
        
        VK::Image albedoImage, normalImage, velocityImage, currentColorImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO;
        imageDescriptor.allocationFlag = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        if (!albedoImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan albedo image");
        }

        imageDescriptor.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        if (!normalImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan normal image");
        }

        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        imageDescriptor.format = VK_FORMAT_R16G16_SNORM;
        if (!velocityImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan velocity image");
        }

        imageDescriptor.format = swapchainImage.getFormat();
        if (!currentColorImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan current color image");
        }

        VK::Image depthImage;
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (!depthImage.init(*_device, imageDescriptor)) {
            throw AN::Exception("cannot init vulkan depth image");
        }

        std::vector<VK::Image> images;
        images.emplace_back(std::move(swapchainImage));

        if (deferredRenderTargets.size() < index + 1) {
            deferredRenderTargets.resize(index + 1);
        }
        deferredRenderTargets[index].clear();
        
        deferredRenderTargets[index].extent = extent;
        deferredRenderTargets[index].images.emplace_back(std::move(depthImage));
        deferredRenderTargets[index].images.emplace_back(std::move(albedoImage));
        deferredRenderTargets[index].images.emplace_back(std::move(normalImage));
        deferredRenderTargets[index].images.emplace_back(std::move(velocityImage));
        deferredRenderTargets[index].images.emplace_back(std::move(currentColorImage));

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

        for (VK::Image &image : deferredRenderTargets[index].images) {
            deferredRenderTargets[index].views.emplace_back();
            if (!deferredRenderTargets[index].views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
                throw AN::Exception("cannot init vulkan image view");
            }

            VK::RenderAttachment attachment;
            attachment.format = image.getFormat();
            attachment.usage = image.getUsage();
            attachment.samples = image.getSampleCount();

            deferredRenderTargets[index].attachments.push_back(attachment);
        }

        layerFrameCount = 0;

        return {
                ._device = _device,
                .extent = extent,
                .images = std::move(images),
                .views = std::move(views),
                .attachments = std::move(renderAttachments),
                .input_attachments = {},
                .output_attachments = { 0 }
        };

    }
    
    void beginGeometryPass(VK::RenderCommandEncoder &renderCommandEncoder, uint32_t index) {
        deferredRenderPassDescriptor.attachments = deferredRenderTargets[index].attachments;
        activeFrameIndex = index;
        renderCommandEncoder.debugLabelBegin("Deferred Pass", { 0.f, 1.f, 0.f, 1.f });
        renderCommandEncoder.debugLabelBegin("Geometry Pass", { 0.f, 0.5f, 0.4f, 1.f });
        
        auto &renderTarget = deferredRenderTargets[index];
        
        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            renderCommandEncoder.imageBarrier(renderTarget.views[0], memory_barrier);
        }

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            for (size_t i = 1; i < renderTarget.views.size(); ++i) {
                renderCommandEncoder.imageBarrier(renderTarget.views[i], memory_barrier);
            }
        }

        deferredRenderPassDescriptor.attachments = renderTarget.attachments;

        renderCommandEncoder.beginRenderPass(renderTarget.extent,
                                             renderTarget.views,
                                             deferredRenderPassDescriptor, deferredClearValue);
    }
    
    
    void nextLightingPass(const RenderContext &renderContext, VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.debugLabelEnd();
        renderCommandEncoder.debugLabelBegin("Lighting Pass", { 0.f, 1.f, 1.f, 1.f });
        renderCommandEncoder.nextSubPass();
        
        auto cameraNode = Node3D::GetCurrentCamera();
        if (cameraNode) {
            renderCommandEncoder.setRenderPipelineState(lightingPipelineState);
            struct LightingGlobalUniform {
                alignas(16) Math::mat4 inv_view_proj;
                alignas(8)  Math::vec2 inv_resolution;
            };
            LightingGlobalUniform light_uniform{};
            // Inverse resolution
            light_uniform.inv_resolution.x = 1.0f / renderContext.frameWidth;
            light_uniform.inv_resolution.y = 1.0f / renderContext.frameHeight;
            // Inverse view projection
            
            light_uniform.inv_view_proj = Math::inverse(cameraNode->getProjectionMatrix() * cameraNode->getViewMatrix());

            renderCommandEncoder.pushConstants(0, sizeof light_uniform, &light_uniform);
            
            auto &renderTarget = deferredRenderTargets[activeFrameIndex];
            renderCommandEncoder.bindImageView(0, renderTarget.views[0]);
            renderCommandEncoder.bindImageView(2, renderTarget.views[1]);
            renderCommandEncoder.bindImageView(3, renderTarget.views[2]);

            renderCommandEncoder.draw(6);
        }
    }
    
    void nextTAAPass(const RenderContext &renderContext, 
                     VK::RenderCommandEncoder &renderCommandEncoder,
                     uint32_t framesInFlight, 
                     const VK::RenderTarget &renderTarget) {
        currentRenderTarget = &renderTarget;
        renderCommandEncoder.endRenderPass();
        
        renderCommandEncoder.debugLabelEnd();
        renderCommandEncoder.debugLabelBegin("TAA Pass", { 1.f, 1.f, 0.f, 1.f });
        
        const auto &deferredRenderTarget = deferredRenderTargets[activeFrameIndex];
        
        uint32_t lastFrame = (activeFrameIndex - 1 + framesInFlight) % framesInFlight;

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            renderCommandEncoder.imageBarrier(renderTarget.views[0], memory_barrier);
        }

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            renderCommandEncoder.imageBarrier(deferredRenderTarget.views[3], memory_barrier);
            renderCommandEncoder.imageBarrier(deferredRenderTarget.views[4], memory_barrier);
        }

        if (layerFrameCount == 0) {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            renderCommandEncoder.imageBarrier(deferredRenderTargets[lastFrame].views[4], memory_barrier);
        }

        
        taaRenderPassDescriptor.attachments = renderTarget.attachments;
        
        VkClearValue clearValue[1];
        clearValue[0].color = { 0.f, 0.f, 0.f, 1.f }; // don't actually need this
        renderCommandEncoder.beginRenderPass(renderTarget.extent,
                                             renderTarget.views,
                                             taaRenderPassDescriptor,
                                             clearValue);

        renderCommandEncoder.setRenderPipelineState(taaPipelineState);

        struct TaaUniform {
            float screenWidth, screenHeight;
        } taaUniform;

        taaUniform.screenWidth = renderContext.frameWidth;
        taaUniform.screenHeight = renderContext.frameHeight;

        renderCommandEncoder.pushConstants(0, sizeof taaUniform, &taaUniform);
        renderCommandEncoder.bindSampler(0, taaSampler);
        renderCommandEncoder.bindImageView(1, deferredRenderTarget.views[0]);
        renderCommandEncoder.bindImageView(2, deferredRenderTarget.views[3]);
        renderCommandEncoder.bindImageView(3, deferredRenderTarget.views[4]);


        renderCommandEncoder.bindImageView(4, deferredRenderTargets[lastFrame].views[4]);
        renderCommandEncoder.draw(6);
        
    }
    
    void endRenderPass(VK::RenderCommandEncoder &renderCommandEncoder) {
        renderCommandEncoder.endRenderPass();
        renderCommandEncoder.debugLabelEnd();
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
        
        ++layerFrameCount;
    }
};

}

#endif//OJOIE_VK_DEFERREDPASS_HPP
