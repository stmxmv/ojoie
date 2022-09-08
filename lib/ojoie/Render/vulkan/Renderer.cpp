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
#include "Render/private/vulkan/RenderFrame.hpp"
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/SwapChain.hpp"

#include "Render/Texture.hpp"
#include "Render/UniformBuffer.hpp"
#include "Render/Sampler.hpp"

#ifdef OJOIE_USE_GLFW
#include <GLFW/glfw3.h>

#endif


#include <unordered_map>
#include <set>

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

    std::unordered_map<Window *, VK::Layer> layers;

    VK::DescriptorSetInfo descriptorSetInfo;
    std::map<uint32_t, uint32_t> uniformBuffersOffsets;
    std::vector<uint32_t> descriptorSetDynamicOffsets;


};

Renderer::Renderer() : impl(new Impl{}){
    renderContext.graphicContext = new GraphicContext{};

    GetConfiguration().setObject("deferred-rendering", true);

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


    renderContext.maxFrameInFlight = MAX_FRAMES_IN_FLIGHT;


    renderContext.graphicContext->gpuProperties = &impl->device.getPhysicalDeviceProperties();
    renderContext.graphicContext->vkInstance = impl->instance.vKInstance();
    renderContext.graphicContext->physicalDevice = impl->instance.getPhysicalDevice();
    renderContext.graphicContext->logicalDevice = impl->device.vkDevice();
    renderContext.graphicContext->vmaAllocator = impl->device.vmaAllocator();

    renderContext.graphicContext->commandPool = impl->device.getCommandPool().vkCommandPool();
    renderContext.graphicContext->graphicsQueueFamily = impl->device.graphicsQueue().getFamilyIndex();
    renderContext.graphicContext->graphicQueue = impl->device.graphicsQueue().vkQueue();

    impl->msaaSamples = getMaxUsableSampleCount(impl->device.getPhysicalDeviceProperties());

    /// TODO
    renderContext.msaaSamples = 1;

    renderContext.graphicContext->descriptorLayoutCache.init(impl->device.vkDevice());

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

    impl->layers.clear();

    renderContext.graphicContext->descriptorLayoutCache.deinit();

    impl->device.deinit();
    impl->instance.deinit();
}

void Renderer::changeNodes(const std::vector<std::shared_ptr<Node>> &nodes) {
    nodesToRender = nodes;
}

void Renderer::render(float deltaTime, float elapsedTime) {
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


            layer.layerCreateRenderTarget = [this](VK::Image &&swapchainImage) -> VK::RenderTarget {
                VkExtent2D extent = { .width = swapchainImage.getExtent().width, .height = swapchainImage.getExtent().height };
                VK::Image depthImage;
                VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
                imageDescriptor.extent = swapchainImage.getExtent();
                imageDescriptor.format = VK_FORMAT_D32_SFLOAT;
                imageDescriptor.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
                imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

                if (!depthImage.init(impl->device, imageDescriptor)) {
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
                        ._device = &impl->device,
                        .extent = extent,
                        .images = std::move(images),
                        .views = std::move(views),
                        .attachments = renderAttachments,
                        .input_attachments = {},
                        .output_attachments = { 0 }
                };
            };


            layer.prepare();
        }
    }


    renderContext.deltaTime = deltaTime;
    renderContext.elapsedTime = elapsedTime;

    renderContext.window = currentWindow;
    renderContext.cursorState = currentCursorState;

    VK::Layer &layer = impl->layers[lastWindow];

    VkCommandBuffer commandBuffer = layer.beginFrame();

    layer.getActiveFrame().getDescriptorSetManager().clearFrameSets();

    renderContext.graphicContext->commandBuffer = commandBuffer;
    renderContext.graphicContext->renderFrame = &layer.getActiveFrame();

    renderContext.frameWidth = (float)layer.getActiveFrame().getRenderTarget().extent.width;
    renderContext.frameHeight = (float)layer.getActiveFrame().getRenderTarget().extent.height;

    /// create renderpass

    std::vector<VK::LoadStoreInfo> load_store{2};

    // Swapchain
    load_store[0].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Depth
    load_store[1].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    std::vector<VK::SubpassInfo> subpass_infos{1};
    subpass_infos[0].outputAttachments = { 0 };

    VK::RenderPassDescriptor renderPassDescriptor;
    renderPassDescriptor.loadStoreInfos = load_store;
    renderPassDescriptor.attachments = layer.getActiveFrame().getRenderTarget().attachments;
    renderPassDescriptor.subpasses = subpass_infos;

    std::vector<VkClearValue> clear_value{2};
    clear_value[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_value[1].depthStencil = { 1.0f, 0 };


    VK::RenderPassCommandEncoder renderPassCommandEncoder(layer, clear_value, commandBuffer, renderPassDescriptor);

    /// TODO remove backward compatibility
    renderContext.graphicContext->renderPass = renderPassCommandEncoder.getRenderPass().vkRenderPass();


    for (auto &node : nodesToRender) {
        if (node->r_needsRender) {
            node->render(renderContext);
        }
    }

    renderPassCommandEncoder.endRenderPass();
    renderPassCommandEncoder.submitAndPresent();

    ++renderContext.frameCount;

    if (completionHandler) [[likely]] {
        completionHandler();
    }

}

void Renderer::didChangeRenderPipeline(class RC::RenderPipeline &pipeline) {
    impl->descriptorSetInfo.clear();
    impl->uniformBuffersOffsets.clear();
    VkDescriptorSetLayout descriptorSetLayout = (VkDescriptorSetLayout) pipeline.getVkDescriptorLayout();
    impl->descriptorSetInfo.layout = descriptorSetLayout;
}

void Renderer::bindUniformBuffer(uint32_t binding, RC::UniformBuffer &uniformBuffer) {
    VkDescriptorBufferInfo &bufferInfo = impl->descriptorSetInfo.bufferInfos[binding];

    bufferInfo.offset = 0;
    bufferInfo.buffer = (VkBuffer)uniformBuffer.getUnderlyingBuffer();
    bufferInfo.range = uniformBuffer.getSize();

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    impl->uniformBuffersOffsets[binding] = uniformBuffer.getOffset();
}

void Renderer::bindTexture(uint32_t binding, RC::Texture &texture) {
    VkDescriptorImageInfo &imageInfo = impl->descriptorSetInfo.imageInfos[binding];
    imageInfo.imageView = (VkImageView)texture.getUnderlyingTexture();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler = nullptr;

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

}

void Renderer::bindSampler(uint32_t binding, RC::Sampler &sampler) {
    VkDescriptorImageInfo &imageInfo = impl->descriptorSetInfo.imageInfos[binding];
    imageInfo.sampler = (VkSampler)sampler.getUnderlyingSampler();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.imageView = nullptr;

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLER;

}

void Renderer::drawIndexed(uint32_t indexCount) {
    impl->descriptorSetDynamicOffsets.clear();
    impl->descriptorSetDynamicOffsets.reserve(impl->uniformBuffersOffsets.size());

    for (auto &[_, offset] : impl->uniformBuffersOffsets) {
        impl->descriptorSetDynamicOffsets.push_back(offset);
    }


    VkDescriptorSet descriptorSet = renderContext.graphicContext->renderFrame->descriptorSet(impl->descriptorSetInfo);

    VkPipelineLayout pipelineLayout = (VkPipelineLayout) RC::RenderPipeline::Current()->getVkPipelineLayout();

    vkCmdBindDescriptorSets(renderContext.graphicContext->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &descriptorSet,
                            impl->descriptorSetDynamicOffsets.size(), impl->descriptorSetDynamicOffsets.data());


    vkCmdDrawIndexed(renderContext.graphicContext->commandBuffer, indexCount, 1, 0, 0, 0);
}

void Renderer::draw(uint32_t count) {

    impl->descriptorSetDynamicOffsets.clear();
    impl->descriptorSetDynamicOffsets.reserve(impl->uniformBuffersOffsets.size());

    for (auto &[_, offset] : impl->uniformBuffersOffsets) {
        impl->descriptorSetDynamicOffsets.push_back(offset);
    }


    VkDescriptorSet descriptorSet = renderContext.graphicContext->renderFrame->descriptorSet(impl->descriptorSetInfo);

    VkPipelineLayout pipelineLayout = (VkPipelineLayout) RC::RenderPipeline::Current()->getVkPipelineLayout();

    vkCmdBindDescriptorSets(renderContext.graphicContext->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &descriptorSet,
                            impl->descriptorSetDynamicOffsets.size(), impl->descriptorSetDynamicOffsets.data());

    vkCmdDraw(renderContext.graphicContext->commandBuffer, count, 1, 0, 0);
}

}