//
// Created by Aleudillonam on 8/12/2022.
//

#include "Render/Renderer.hpp"
#include "Core/Window.hpp"
#include "Core/Configuration.hpp"
#include "Render/private/vulkan.hpp"

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan/Instance.hpp"
#include "Render/private/vulkan/Layer.hpp"
#include "Render/private/vulkan/RenderCommandEncoder.hpp"
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/CommandQueue.hpp"
#include "Render/private/vulkan/BufferPool.hpp"

#include "Render/UniformBuffer.hpp"
#include "Template/Access.hpp"


#ifdef OJOIE_USE_GLFW
#include <GLFW/glfw3.h>

#endif


#include <unordered_map>
#include <set>


#define BUFFER_POOL_BLOCK_SIZE (256 * 1024)




namespace AN {
namespace {

struct DeviceImplTag : Access::TagBase<DeviceImplTag> {};

struct CommandQueueImplTag : Access::TagBase<CommandQueueImplTag> {};

struct BufferPoolImplTag : Access::TagBase<BufferPoolImplTag> {};
struct BufferPoolShouldFreeTag : Access::TagBase<BufferPoolShouldFreeTag> {};

struct BlitCommandEncoderImplTag : Access::TagBase<BlitCommandEncoderImplTag> {};
struct BlitCommandEncoderShouldFreeTag : Access::TagBase<BlitCommandEncoderShouldFreeTag> {};

struct RenderCommandEncoderImplTag : Access::TagBase<RenderCommandEncoderImplTag> {};
}

template struct Access::Accessor<DeviceImplTag, &RC::Device::impl>;
template struct Access::Accessor<CommandQueueImplTag, &RC::CommandQueue::impl>;

template struct Access::Accessor<BufferPoolImplTag, &RC::BufferPool::impl>;
template struct Access::Accessor<BufferPoolShouldFreeTag, &RC::BufferPool::shouldFree>;

template struct Access::Accessor<BlitCommandEncoderImplTag, &RC::BlitCommandEncoder::impl>;
template struct Access::Accessor<BlitCommandEncoderShouldFreeTag, &RC::BlitCommandEncoder::shouldFree>;

template struct Access::Accessor<RenderCommandEncoderImplTag, &RC::RenderCommandEncoder::impl>;
}

namespace AN {


VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties &physicalDeviceProperties) {

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

#define MAX_FRAMES_IN_FLIGHT 3




struct Renderer::Impl {

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VK::Instance instance;
    VK::Device device;

    VK::CommandQueue commandQueue; // an commandQueue associated with primary queue

    VK::CommandBuffer initCommandBuffer;

    std::unordered_map<Window *, VK::Layer> layers;

    VK::RenderTarget createDefaultRenderTarget(VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
        VK::Image depthImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!depthImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan depth image");
        }

        std::vector<VK::Image> images;

        images.emplace_back(std::move(swapchainImage));
        images.emplace_back(std::move(depthImage));


        std::vector<VK::ImageView> views;

        std::vector<VK::RenderAttachment> renderAttachments;

        for (VK::Image &image : images) {
            views.emplace_back();
            if (!views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
                throw Exception("cannot init vulkan image view");
            }

            VK::RenderAttachment attachment;
            attachment.format = image.getFormat();
            attachment.usage = image.getUsage();
            attachment.samples = image.getSampleCount();

            renderAttachments.push_back(attachment);
        }

        return {
                ._device = &device,
                .extent = extent,
                .images = std::move(images),
                .views = std::move(views),
                .attachments = renderAttachments,
                .input_attachments = {},
                .output_attachments = { 0 }
        };
    }

    VK::RenderTarget createMSAARenderTarget(VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };

        VK::Image msaaImage;
        VK::ImageDescriptor msaaImageDescriptror = VK::ImageDescriptor::Default2D();
        msaaImageDescriptror.extent = swapchainImage.getExtent();
        msaaImageDescriptror.format = swapchainImage.getFormat();
        msaaImageDescriptror.imageUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        msaaImageDescriptror.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        msaaImageDescriptror.sampleCount = msaaSamples;

        if (!msaaImage.init(device,msaaImageDescriptror)) {
            throw Exception("cannot init vulkan msaa image");
        }


        VK::Image depthImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.sampleCount = msaaSamples;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!depthImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan depth image");
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
                throw Exception("cannot init vulkan image view");
            }

            VK::RenderAttachment attachment;
            attachment.format = image.getFormat();
            attachment.usage = image.getUsage();
            attachment.samples = image.getSampleCount();

            renderAttachments.push_back(attachment);
        }

        return {
                ._device = &device,
                .extent = extent,
                .images = std::move(images),
                .views = std::move(views),
                .attachments = renderAttachments,
                .input_attachments = {},
                .output_attachments = { 0 }
        };

    }

    VK::RenderTarget layerCreateRenderTarget(VK::Image &&swapchainImage) {
        if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "MSAA") == 0) {

            return createMSAARenderTarget(std::move(swapchainImage));

        } else {

            return createDefaultRenderTarget(std::move(swapchainImage));
        }
    }

};

Renderer::Renderer() : impl(new Impl{}){
    renderContext.graphicContext = new GraphicContext{};

    GetConfiguration().setObject("deferred-rendering", false);

    GetConfiguration().setObject("anti-aliasing", "MSAA");

}

Renderer::~Renderer() {
    delete renderContext.graphicContext;
    delete impl;
}

Renderer &Renderer::GetSharedRenderer() {
    static Renderer renderer;
    return renderer;
}

bool Renderer::init() {

    VK::InstanceDescriptor instanceDescriptor;
    instanceDescriptor.headless = false;
    instanceDescriptor.applicationName = "ojoie game instance";


#ifdef _WIN32
    instanceDescriptor.requiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else

#error "not implement"

#endif

    if (!impl->instance.init(instanceDescriptor)) {
        return false;
    }


    VkPhysicalDeviceFeatures deviceFeatures {
            .geometryShader = true,
            .sampleRateShading = true,
            .samplerAnisotropy = true
    };

    VK::DeviceDescriptor deviceDescriptor;
    deviceDescriptor.instance = impl->instance.vKInstance();
    deviceDescriptor.physicalDevice = impl->instance.getPhysicalDevice();
    deviceDescriptor.requestedFeatures = deviceFeatures;
    deviceDescriptor.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkSurfaceKHR testSurface;

#ifdef OJOIE_USE_GLFW

    if (glfwCreateWindowSurface(impl->instance.vKInstance(),
                                (GLFWwindow *) currentWindow.load(std::memory_order_relaxed)->getUnderlyingWindow(),
                                nullptr, &testSurface) != VK_SUCCESS) {
        ANLog("failed to create window surface!");
        return false;
    }
#else

#error "not implement"

#endif

    deviceDescriptor.surface = testSurface;
    deviceDescriptor.onlyOneDevice = true;

    if (!impl->device.init(deviceDescriptor)) {
        return false;
    }

    vkDestroySurfaceKHR(impl->instance.vKInstance(), testSurface, nullptr);

    if (!impl->commandQueue.init(impl->device,
                                 impl->device.queue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0),
                                 VK::CommandBufferResetMode::ResetIndividually)) {
        return false;
    }

    renderContext.maxFrameInFlight = MAX_FRAMES_IN_FLIGHT;

    Access::set<DeviceImplTag>(renderContext.device, &impl->device);
    renderContext.graphicContext->device = &impl->device;

    Access::set<CommandQueueImplTag>(renderContext.commandQueue, &impl->commandQueue);

    /// provide a blitCommandEncoder to init some render stuff
    impl->initCommandBuffer = impl->commandQueue.commandBuffer();
    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, new VK::BlitCommandEncoder(impl->initCommandBuffer.vkCommandBuffer()));
    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, true);

    VK::BufferPool *bufferPool = new VK::BufferPool();
    if (!bufferPool->init(impl->device, BUFFER_POOL_BLOCK_SIZE,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)) {

        return false;
    }

    Access::set<BufferPoolImplTag>(renderContext.stageBufferPool, bufferPool);
    Access::set<BufferPoolShouldFreeTag>(renderContext.stageBufferPool, true);

    impl->msaaSamples = getMaxUsableSampleCount(impl->device.getPhysicalDeviceProperties());

    if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "MSAA") == 0) {

        renderContext.msaaSamples = impl->msaaSamples;

    } else {
        renderContext.msaaSamples = 1;
    }


    return true;
}


void Renderer::willDeinit() {
    /// called before resources deinit
    resourceFence();
    isStop = true;
}

void Renderer::resourceFence() {
    if (!isStop) {
        impl->device.graphicsQueue().waitIdle();
    }
}

void Renderer::deinit() {
    impl->device.waitIdle();

    renderContext.stageBufferPool.deinit();
    renderContext.vertexBufferPool.deinit();
    renderContext.indexBufferPool.deinit();
    impl->layers.clear();

    impl->commandQueue.deinit();
    impl->device.deinit();
    impl->instance.deinit();
}

void Renderer::changeNodes(const std::vector<std::shared_ptr<Node>> &nodes) {
    nodesToRender = nodes;
}

void Renderer::render(float deltaTime, float elapsedTime) {
    static bool firstTime = true;
    if (firstTime) [[unlikely]] {
        /// submit the init procedure command
        impl->initCommandBuffer.submit();
        impl->initCommandBuffer.waitUntilCompleted();

        renderContext.commandQueue.reset();



        firstTime = false;
    }

    static Window *lastWindow = nullptr;
    if (lastWindow != currentWindow.load(std::memory_order_relaxed)) [[unlikely]] {
        lastWindow = currentWindow.load(std::memory_order_relaxed);

        auto layerIter = impl->layers.find(lastWindow);
        if (!impl->layers.contains(lastWindow)) [[unlikely]] {
            VK::Layer &layer = impl->layers[lastWindow];

            if (!layer.init(impl->device, lastWindow)) {
                impl->layers.erase(lastWindow);
                return;
            }

            layer.layerCreateRenderTarget.bind(impl, &Impl::layerCreateRenderTarget);

            layer.prepare();
        }
    }


    renderContext.deltaTime = deltaTime;
    renderContext.elapsedTime = elapsedTime;

    renderContext.window = currentWindow;
    renderContext.cursorState = currentCursorState;

    VK::Layer &layer = impl->layers[lastWindow];

    VK::Presentable currentPresentable = layer.nextPresentable();

    /// skip this frame
    if (currentPresentable.swapchain == nullptr) {
        return;
    }

    VK::CommandBuffer commandBuffer = layer.getActiveFrame().commandBuffer(impl->device.queue(VK_QUEUE_GRAPHICS_BIT, 0),
                                                                           VK::CommandBufferResetMode::ResetPool,
                                                                           VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VK::CommandBuffer blitCommandBuffer = layer.getActiveFrame().commandBuffer(impl->device.queue(VK_QUEUE_GRAPHICS_BIT, 0),
                                                                           VK::CommandBufferResetMode::ResetPool,
                                                                               VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    renderContext.graphicContext->commandBuffer = commandBuffer.vkCommandBuffer();



    renderContext.frameWidth = (float)layer.getActiveFrame().getRenderTarget().extent.width;
    renderContext.frameHeight = (float)layer.getActiveFrame().getRenderTarget().extent.height;

    /// create renderpass

    VK::RenderPassDescriptor renderPassDescriptor;

    renderPassDescriptor.attachments = layer.getActiveFrame().getRenderTarget().attachments;

    std::vector<VK::SubpassInfo> subpass_infos;
    std::vector<VkClearValue> clear_value;

    if (renderContext.msaaSamples == 1) {
        std::vector<VK::LoadStoreInfo> load_store{2};

        // Swapchain
        load_store[0].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Depth
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        subpass_infos.resize(1);
        subpass_infos[0].outputAttachments = { 0 };

        renderPassDescriptor.loadStoreInfos = load_store;

        clear_value.resize(2);
        clear_value[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        clear_value[1].depthStencil = { 1.0f, 0 };

    } else {

        std::vector<VK::LoadStoreInfo> load_store(3);

        // resolve swapchain image
        load_store[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; /// 0 swapchain image will be resolved anyway
        load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Depth
        load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // msaa image
        load_store[2].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        load_store[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        subpass_infos.resize(1);
        subpass_infos[0].outputAttachments = { 2 };
        subpass_infos[0].resolveAttachment = 0;

        renderPassDescriptor.loadStoreInfos = load_store;

        clear_value.resize(3);

        clear_value[1].depthStencil = { 1.0f, 0 };
        clear_value[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

    }




    renderPassDescriptor.subpasses = subpass_infos;

    VK::RenderCommandEncoder renderCommandEncoder(impl->device,
                                                  commandBuffer.vkCommandBuffer(),
                                                  layer.getActiveFrame().getRenderTarget(),
                                                  renderPassDescriptor,
                                                  layer.getActiveFrame().getDescriptorSetManager());

    renderContext.graphicContext->renderCommandEncoder = &renderCommandEncoder;

    Access::set<RenderCommandEncoderImplTag>(renderContext.renderCommandEncoder, &renderCommandEncoder);

    std::destroy_at(&renderContext.blitCommandEncoder);
    std::construct_at(&renderContext.blitCommandEncoder);

    VK::BlitCommandEncoder blitCommandEncoder(blitCommandBuffer.vkCommandBuffer());

    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, &blitCommandEncoder);
    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, false);

    std::destroy_at(&renderContext.stageBufferPool);
    std::destroy_at(&renderContext.vertexBufferPool);
    std::destroy_at(&renderContext.indexBufferPool);

    std::construct_at(&renderContext.stageBufferPool);
    std::construct_at(&renderContext.vertexBufferPool);
    std::construct_at(&renderContext.indexBufferPool);

    Access::set<BufferPoolImplTag>(renderContext.stageBufferPool, &layer.getActiveFrame().getStageBufferPool());
    Access::set<BufferPoolImplTag>(renderContext.vertexBufferPool, &layer.getActiveFrame().getVertexBufferPool());
    Access::set<BufferPoolImplTag>(renderContext.indexBufferPool, &layer.getActiveFrame().getIndexBufferPool());

    Access::set<BufferPoolShouldFreeTag>(renderContext.stageBufferPool, false);
    Access::set<BufferPoolShouldFreeTag>(renderContext.vertexBufferPool, false);
    Access::set<BufferPoolShouldFreeTag>(renderContext.indexBufferPool, false);

    const auto &views = layer.getActiveFrame().getRenderTarget().views;

    {
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.srcAccessMask = 0;
        memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // Skip 1 as it is handled later as a depth-stencil attachment

        renderCommandEncoder.imageBarrier(views[0], memory_barrier);

        for (size_t i = 2; i < views.size(); ++i) {
            renderCommandEncoder.imageBarrier(views[i], memory_barrier);
        }
    }

    {
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        memory_barrier.srcAccessMask = 0;
        memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        renderCommandEncoder.imageBarrier(views[1], memory_barrier);
    }

    renderCommandEncoder.beginRenderPass(clear_value);


    renderCommandEncoder.setViewport({ .originX = 0.f, .originY = 0.f,
                                       .width = renderContext.frameWidth, .height = renderContext.frameHeight,
                                       .znear = 0.f, .zfar = 1.f  });

    renderCommandEncoder.setScissor({ .x = 0, .y = 0, .width = (int)renderContext.frameWidth, .height = (int)renderContext.frameHeight });

//    renderCommandEncoder.setCullMode(RC::CullMode::Back);

    for (auto &node : nodesToRender) {
        if (node->r_needsRender) {
            node->render(renderContext);
        }
    }

    renderCommandEncoder.endRenderPass();


    {
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        renderCommandEncoder.imageBarrier(views[0], memory_barrier);
    }

    blitCommandBuffer.submit();

    commandBuffer.presentDrawable(currentPresentable);

    commandBuffer.submit();


    impl->commandQueue.reset(); // reset the commandQueue if frame makes any commandBuffer

    ++renderContext.frameCount;

    if (completionHandler) [[likely]] {
        completionHandler();
    }

}




}