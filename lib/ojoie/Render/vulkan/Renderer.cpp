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

#include "Node/Node3D.hpp"
#include "Node/CameraNode.hpp"
#include "Math/Math.hpp"

#include "Template/Access.hpp"


#ifdef OJOIE_USE_GLFW
#include <GLFW/glfw3.h>

#endif


#include <unordered_map>
#include <set>
#include <filesystem>
#include <fstream>

#define BUFFER_POOL_BLOCK_SIZE (10 << 20)




namespace AN {
namespace {

struct DeviceImplTag : Access::TagBase<DeviceImplTag> {};

struct CommandQueueImplTag : Access::TagBase<CommandQueueImplTag> {};

struct BufferPoolImplTag : Access::TagBase<BufferPoolImplTag> {};
struct BufferPoolShouldFreeTag : Access::TagBase<BufferPoolShouldFreeTag> {};

struct BufferManagerImplTag : Access::TagBase<BufferManagerImplTag> {};
struct BufferManagerShouldFreeTag : Access::TagBase<BufferManagerShouldFreeTag> {};

struct BlitCommandEncoderImplTag : Access::TagBase<BlitCommandEncoderImplTag> {};
struct BlitCommandEncoderShouldFreeTag : Access::TagBase<BlitCommandEncoderShouldFreeTag> {};

struct RenderCommandEncoderImplTag : Access::TagBase<RenderCommandEncoderImplTag> {};
}

template struct Access::Accessor<DeviceImplTag, &RC::Device::impl>;
template struct Access::Accessor<CommandQueueImplTag, &RC::CommandQueue::impl>;

template struct Access::Accessor<BufferPoolImplTag, &RC::BufferPool::impl>;
template struct Access::Accessor<BufferPoolShouldFreeTag, &RC::BufferPool::shouldFree>;

template struct Access::Accessor<BufferManagerImplTag, &RC::BufferManager::impl>;
template struct Access::Accessor<BufferManagerShouldFreeTag, &RC::BufferManager::shouldFree>;

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


struct LightingGlobalUniform {
    alignas(16) Math::mat4 inv_view_proj;
    alignas(8)  Math::vec2 inv_resolution;
};

struct Light {
    alignas(16) Math::vec4 lightPos;
    alignas(16) Math::vec4 lightColor;
};

struct Renderer::Impl {

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VK::Instance instance;
    VK::Device device;

    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };

    VK::CommandQueue commandQueue; // an commandQueue associated with primary queue

    VK::CommandBuffer initCommandBuffer;

    std::unordered_map<Window *, VK::Layer> layers;

    VK::RenderPipelineState lightingPipelineState;

    RenderContext *context;

    struct TAAFrame {
        std::vector<VK::Image> images;
        std::vector<VK::ImageView> views;

        std::vector<VK::RenderAttachment> attachment;

        void clear() {
            images.clear();
            views.clear();
            attachment.clear();
        }
    };

    VK::RenderPipelineState taaPipelineState;
    std::vector<TAAFrame> taaFrames{ MAX_FRAMES_IN_FLIGHT };
    RC::Sampler taaSampler;

    uint32_t layerFrameCount{}; // when layer recreate its render targets, reset this to zero

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

    VK::RenderTarget createDeferredRenderTarget(VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
        VK::Image albedoImage, normalImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!albedoImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan albedo image");
        }

        imageDescriptor.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        if (!normalImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan normal image");
        }

        VK::Image depthImage;
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (!depthImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan depth image");
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

    VK::RenderTarget createDeferredTAARenderTarget(uint32_t index, VK::Image &&swapchainImage) {
        VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };

        VK::Image albedoImage, normalImage, velocityImage, currentColorImage;
        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
        imageDescriptor.extent = swapchainImage.getExtent();
        imageDescriptor.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO;
        imageDescriptor.allocationFlag = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        if (!albedoImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan albedo image");
        }

        imageDescriptor.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        if (!normalImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan normal image");
        }

        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        imageDescriptor.format = VK_FORMAT_R16G16_SNORM;
        if (!velocityImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan velocity image");
        }

        imageDescriptor.format = swapchainImage.getFormat();
        if (!currentColorImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan current color image");
        }

        VK::Image depthImage;
        imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
        imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (!depthImage.init(device, imageDescriptor)) {
            throw Exception("cannot init vulkan depth image");
        }

        std::vector<VK::Image> images;
        images.emplace_back(std::move(swapchainImage));

        taaFrames[index].clear();

        taaFrames[index].images.emplace_back(std::move(depthImage));
        taaFrames[index].images.emplace_back(std::move(albedoImage));
        taaFrames[index].images.emplace_back(std::move(normalImage));
        taaFrames[index].images.emplace_back(std::move(velocityImage));
        taaFrames[index].images.emplace_back(std::move(currentColorImage));

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

        for (VK::Image &image : taaFrames[index].images) {
            taaFrames[index].views.emplace_back();
            if (!taaFrames[index].views.back().init(image, VK_IMAGE_VIEW_TYPE_2D, image.getFormat())) {
                throw Exception("cannot init vulkan image view");
            }

            VK::RenderAttachment attachment;
            attachment.format = image.getFormat();
            attachment.usage = image.getUsage();
            attachment.samples = image.getSampleCount();

            taaFrames[index].attachment.push_back(attachment);
        }

        layerFrameCount = 0;

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

    VK::RenderTarget layerCreateRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
        if (!context->forwardShading) {
            if (context->antiAliasing == AntiAliasingMethod::TAA) {
                return createDeferredTAARenderTarget(index, std::move(swapchainImage));
            } else {
                return createDeferredRenderTarget(std::move(swapchainImage));
            }

        } else {
            if (context->antiAliasing == AntiAliasingMethod::MSAA) {
                return createMSAARenderTarget(std::move(swapchainImage));
            }
        }

        return createDefaultRenderTarget(std::move(swapchainImage));
    }

};

Renderer::Renderer() : impl(new Impl{}){
    renderContext.graphicContext = new GraphicContext{};

    GetConfiguration().setObject("forward-shading", false);

    GetConfiguration().setObject("anti-aliasing", "TAA");

    impl->context = &renderContext;
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
            .independentBlend = true,
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

    renderContext.frameWidth = renderContext.windowWidth;
    renderContext.frameHeight = renderContext.windowHeight;

    renderContext.maxFrameInFlight = MAX_FRAMES_IN_FLIGHT;

    Access::set<DeviceImplTag>(renderContext.device, &impl->device);
    renderContext.graphicContext->device = &impl->device;

    Access::set<CommandQueueImplTag>(renderContext.commandQueue, &impl->commandQueue);

    /// provide a blitCommandEncoder to init some render stuff
    impl->initCommandBuffer = impl->commandQueue.commandBuffer();
    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, new VK::BlitCommandEncoder(impl->initCommandBuffer.vkCommandBuffer()));
    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, true);

    VkBufferUsageFlagBits supportedUsageFlags[] = { VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
    VK::BufferManager *bufferManager = new VK::BufferManager();
    if (!bufferManager->init(impl->device, supportedUsageFlags, BUFFER_POOL_BLOCK_SIZE)) {
        return false;
    }

    Access::set<BufferManagerImplTag>(renderContext.bufferManager, bufferManager);
    Access::set<BufferManagerShouldFreeTag>(renderContext.bufferManager, true);

    impl->msaaSamples = getMaxUsableSampleCount(impl->device.getPhysicalDeviceProperties());


    if (GetConfiguration().getObject<bool>("forward-shading")) {
        renderContext.forwardShading = true;
        if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "MSAA") == 0) {

            renderContext.msaaSamples = impl->msaaSamples;
            renderContext.antiAliasing = AntiAliasingMethod::MSAA;
        } else {
            renderContext.msaaSamples = 1;
        }

    } else {
        renderContext.forwardShading = false;
        renderContext.msaaSamples = 1;

        if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "TAA") == 0) {
            renderContext.antiAliasing = AntiAliasingMethod::TAA;


            RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();
            samplerDescriptor.addressModeU = RC::SamplerAddressMode::ClampToEdge;
            samplerDescriptor.addressModeV = RC::SamplerAddressMode::ClampToEdge;
            samplerDescriptor.addressModeW = RC::SamplerAddressMode::ClampToEdge;

            ANAssert(impl->taaSampler.init(samplerDescriptor));
        }

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


        ANAssert(impl->lightingPipelineState.init(renderPipelineStateDescriptor));

        if (renderContext.antiAliasing == AntiAliasingMethod::TAA) {
            renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "TAA.vert.spv" };
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "TAA.frag.spv" };
            ANAssert(impl->taaPipelineState.init(renderPipelineStateDescriptor));
        }
    }

    /// retrieve pipeline cache
    VkPipelineCacheCreateInfo create_info{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

    std::vector<uint8_t> cacheData;
    if (std::filesystem::exists("./Caches/pipelineCache")) {
        std::ifstream cacheFile("./Caches/pipelineCache", std::ios::binary);
        if (cacheFile.is_open()) {
            cacheFile.seekg(0, std::ios::end);
            uint64_t read_count = static_cast<uint64_t>(cacheFile.tellg());
            cacheFile.seekg(0, std::ios::beg);

            cacheData.resize(read_count);
            cacheFile.read(reinterpret_cast<char *>(cacheData.data()), (std::streamsize)read_count);
            cacheFile.close();

            create_info.initialDataSize = cacheData.size();
            create_info.pInitialData    = cacheData.data();

        } else {
            ANLog("./Caches/pipelineCache could not open");
        }
    }


    if (VK_SUCCESS != vkCreatePipelineCache(impl->device.vkDevice(), &create_info, nullptr, &impl->pipelineCache)) {
        ANLog("fail to create vulkan pipeline cache");
    }

    impl->device.getRenderResourceCache().setPipelineCache(impl->pipelineCache);

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

    if (impl->pipelineCache) {
        size_t size;
        if (VkResult result = vkGetPipelineCacheData(impl->device.vkDevice(), impl->pipelineCache, &size, nullptr);
            result != VK_SUCCESS) {
            ANLog("vkGetPipelineCacheData return %s", VK::ResultCString(result));
        }

        /* Get data of pipeline cache */
        std::vector<uint8_t> data(size);
        if (VkResult result = vkGetPipelineCacheData(impl->device.vkDevice(), impl->pipelineCache, &size, data.data());
            result != VK_SUCCESS) {
            ANLog("vkGetPipelineCacheData return %s", VK::ResultCString(result));
        }

        /* Write pipeline cache data to a file in binary format */
        if (!std::filesystem::exists("./Caches") || !std::filesystem::is_directory("./Caches")) {
            std::filesystem::create_directories("./Caches");
        }

        std::ofstream cacheFile("./Caches/pipelineCache", std::ios::binary | std::ios::trunc);
        cacheFile.write((const char *)data.data(), (std::streamsize)data.size());
        cacheFile.close();

        /* Destroy Vulkan pipeline cache */
        vkDestroyPipelineCache(impl->device.vkDevice(), impl->pipelineCache, nullptr);

        impl->pipelineCache = nullptr;
    }

    impl->lightingPipelineState.deinit();
    impl->taaFrames.clear();
    impl->taaPipelineState.deinit();
    impl->taaSampler.deinit();

    renderContext.bufferManager.deinit();
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

    std::vector<VK::SubpassInfo> subpass_infos;
    std::vector<VkClearValue> clear_value;

    if (renderContext.forwardShading) {
        if (renderContext.msaaSamples == 1) {
            std::vector<VK::LoadStoreInfo> load_store{2};

            // Swapchain
            load_store[0].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            // Depth
            load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            subpass_infos.resize(1);
            subpass_infos[0].colorAttachments = { 0 };
            subpass_infos[0].depthStencilAttachment = 1;

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
            subpass_infos[0].colorAttachments = { 2 };
            subpass_infos[0].resolveAttachment = 0;
            subpass_infos[0].depthStencilAttachment = 1;

            renderPassDescriptor.loadStoreInfos = load_store;

            clear_value.resize(3);

            clear_value[1].depthStencil = { 1.0f, 0 };
            clear_value[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

        }
    } else {

        if (renderContext.antiAliasing == AntiAliasingMethod::TAA) {
            /// deferred shading
            std::vector<VK::LoadStoreInfo> load_store(5);

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

            subpass_infos.resize(2);
            subpass_infos[0].colorAttachments = { 1, 2, 3 };
            subpass_infos[0].depthStencilAttachment = 1;

            subpass_infos[1].inputAttachments = { 0, 1, 2 };
            subpass_infos[1].colorAttachments = { 4 };

            renderPassDescriptor.loadStoreInfos = load_store;

            clear_value.resize(5);

            clear_value[0].depthStencil = { 1.0f, 0 };
            clear_value[1].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            clear_value[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            clear_value[3].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            clear_value[4].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

        } else {

            /// deferred shading
            std::vector<VK::LoadStoreInfo> load_store(4);

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

            subpass_infos.resize(2);
            subpass_infos[0].colorAttachments = { 2, 3 };
            subpass_infos[0].depthStencilAttachment = 1;

            subpass_infos[1].inputAttachments = { 1, 2, 3 };
            subpass_infos[1].colorAttachments = { 0 };

            renderPassDescriptor.loadStoreInfos = load_store;

            clear_value.resize(4);

            clear_value[1].depthStencil = { 1.0f, 0 };
            clear_value[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            clear_value[3].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        }
    }



    renderPassDescriptor.subpasses = subpass_infos;

    VK::RenderCommandEncoder renderCommandEncoder(impl->device,
                                                  commandBuffer.vkCommandBuffer(),
                                                  layer.getActiveFrame().getDescriptorSetManager());

    renderContext.graphicContext->renderCommandEncoder = &renderCommandEncoder;

    Access::set<RenderCommandEncoderImplTag>(renderContext.renderCommandEncoder, &renderCommandEncoder);

    std::destroy_at(&renderContext.blitCommandEncoder);
    std::construct_at(&renderContext.blitCommandEncoder);

    VK::BlitCommandEncoder blitCommandEncoder(blitCommandBuffer.vkCommandBuffer());

    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, &blitCommandEncoder);
    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, false);

    std::destroy_at(&renderContext.bufferManager);
    std::construct_at(&renderContext.bufferManager);

    Access::set<BufferManagerImplTag>(renderContext.bufferManager, &layer.getActiveFrame().getBufferManager());
    Access::set<BufferManagerShouldFreeTag>(renderContext.bufferManager, false);

    const auto &renderTarget = layer.getActiveFrame().getRenderTarget();

    if (renderContext.antiAliasing != AntiAliasingMethod::TAA) {
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

            for (size_t i = 2; i < renderTarget.views.size(); ++i) {
                renderCommandEncoder.imageBarrier(renderTarget.views[i], memory_barrier);
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

            renderCommandEncoder.imageBarrier(renderTarget.views[1], memory_barrier);
        }

        renderPassDescriptor.attachments = renderTarget.attachments;

        renderCommandEncoder.beginRenderPass(renderTarget.extent,
                                             renderTarget.views,
                                             renderPassDescriptor, clear_value);
    } else {

        auto &taaFrame = impl->taaFrames[layer.getActiveFrameIndex()];

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            renderCommandEncoder.imageBarrier(taaFrame.views[0], memory_barrier);
        }

        {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            for (size_t i = 1; i < taaFrame.views.size(); ++i) {
                renderCommandEncoder.imageBarrier(taaFrame.views[i], memory_barrier);
            }
        }

        renderPassDescriptor.attachments = taaFrame.attachment;

        renderCommandEncoder.beginRenderPass(renderTarget.extent,
                                             taaFrame.views,
                                             renderPassDescriptor, clear_value);
    }


    renderCommandEncoder.setViewport({ .originX = 0.f, .originY = 0.f,
                                       .width = renderContext.frameWidth, .height = renderContext.frameHeight,
                                       .znear = 0.f, .zfar = 1.f  });

    renderCommandEncoder.setScissor({ .x = 0, .y = 0, .width = (int)renderContext.frameWidth, .height = (int)renderContext.frameHeight });

//    renderCommandEncoder.setCullMode(RC::CullMode::Back);

    VK::BufferAllocation uniformAllocation = layer.getActiveFrame().getBufferManager().buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Light));
    Light *uniform = (Light *)uniformAllocation.map();
    uniform->lightPos = { 1.2f, 30.0f, 2.0f, 0.f };
    uniform->lightColor = { 0.980f, 0.976f, 0.902f, 0.01f };

    renderCommandEncoder.bindUniformBuffer(1, uniformAllocation.getOffset(), sizeof(Light), uniformAllocation.getBuffer());

    for (auto &node : nodesToRender) {
        if (node->r_needsRender) {
            node->render(renderContext);
        }
        if (node->r_postRender) {
            postRenderNodes.push_back(node);
        }
    }

    if (! renderContext.forwardShading) {
        renderCommandEncoder.nextSubPass();

        auto cameraNode = Node3D::GetCurrentCamera();
        if (cameraNode) {
            renderCommandEncoder.setRenderPipelineState(impl->lightingPipelineState);

            LightingGlobalUniform light_uniform{};
            // Inverse resolution
            light_uniform.inv_resolution.x = 1.0f / renderContext.frameWidth;
            light_uniform.inv_resolution.y = 1.0f / renderContext.frameHeight;
            // Inverse view projection

            light_uniform.inv_view_proj = Math::inverse(cameraNode->getProjectionMatrix() * cameraNode->getViewMatrix());

            renderCommandEncoder.pushConstants(0, sizeof light_uniform, &light_uniform);

            if (renderContext.antiAliasing == AntiAliasingMethod::TAA) {
                const auto &taaFrame = impl->taaFrames[layer.getActiveFrameIndex()];
                renderCommandEncoder.bindImageView(0, taaFrame.views[0]);
                renderCommandEncoder.bindImageView(2, taaFrame.views[1]);
                renderCommandEncoder.bindImageView(3, taaFrame.views[2]);

            } else {
                renderCommandEncoder.bindImageView(0, renderTarget.views[1]);
                renderCommandEncoder.bindImageView(2, renderTarget.views[2]);
                renderCommandEncoder.bindImageView(3, renderTarget.views[3]);
            }

            renderCommandEncoder.draw(6);
        }
    }

    if (renderContext.antiAliasing == AntiAliasingMethod::TAA) {
        renderCommandEncoder.endRenderPass();

        const auto &taaFrame = impl->taaFrames[layer.getActiveFrameIndex()];

        uint32_t lastFrame = (layer.getActiveFrameIndex() - 1 + layer.framesInFlight()) % layer.framesInFlight();

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

            renderCommandEncoder.imageBarrier(taaFrame.views[3], memory_barrier);
            renderCommandEncoder.imageBarrier(taaFrame.views[4], memory_barrier);
        }

        if (impl->layerFrameCount == 0) {
            VK::ImageMemoryBarrier memory_barrier{};
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            renderCommandEncoder.imageBarrier(impl->taaFrames[lastFrame].views[4], memory_barrier);
        }

        VK::SubpassInfo subpassInfo{};
        subpassInfo.colorAttachments = { 0 };

        VK::LoadStoreInfo loadStoreInfo{};
        loadStoreInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we draw the whole screen
        loadStoreInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        renderPassDescriptor.subpasses = { subpassInfo };
        renderPassDescriptor.attachments = renderTarget.attachments;
        renderPassDescriptor.loadStoreInfos = { loadStoreInfo };

        VkClearValue clearValue[1];
        clearValue[0].color = { 0.f, 0.f, 0.f, 1.f }; // don't actually need this
        renderCommandEncoder.beginRenderPass(renderTarget.extent,
                                             renderTarget.views,
                                             renderPassDescriptor,
                                             clearValue);

        renderCommandEncoder.setRenderPipelineState(impl->taaPipelineState);

        struct TaaUniform {
            float screenWidth, screenHeight;
        } taaUniform;

        taaUniform.screenWidth = renderContext.frameWidth;
        taaUniform.screenHeight = renderContext.frameHeight;

        renderCommandEncoder.pushConstants(0, sizeof taaUniform, &taaUniform);
        renderCommandEncoder.bindSampler(0, impl->taaSampler);
        renderCommandEncoder.bindImageView(1, taaFrame.views[0]);
        renderCommandEncoder.bindImageView(2, taaFrame.views[3]);
        renderCommandEncoder.bindImageView(3, taaFrame.views[4]);


        renderCommandEncoder.bindImageView(4, impl->taaFrames[lastFrame].views[4]);
        renderCommandEncoder.draw(6);

    }

    for (auto &node : postRenderNodes) {
        node->postRender(renderContext);
    }

    postRenderNodes.clear();

    renderCommandEncoder.endRenderPass();


    {
        VK::ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        renderCommandEncoder.imageBarrier(renderTarget.views[0], memory_barrier);
    }

    blitCommandBuffer.submit();

    commandBuffer.presentDrawable(currentPresentable);

    commandBuffer.submit();


    impl->commandQueue.reset(); // reset the commandQueue if frame makes any commandBuffer

    ++renderContext.frameCount;
    ++impl->layerFrameCount;

    if (completionHandler) [[likely]] {
        completionHandler();
    }

}




}