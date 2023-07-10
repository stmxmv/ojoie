//
// Created by Aleudillonam on 8/12/2022.
//

#include "Render/Renderer.hpp"
#include "Core/Window.hpp"
#include "Core/Configuration.hpp"
#include "Render/Scene.hpp"
#include "Render/private/vulkan.hpp"

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan/Instance.hpp"
#include "Render/private/vulkan/Layer.hpp"
#include "Render/private/vulkan/RenderCommandEncoder.hpp"
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/BufferPool.hpp"

#include "Node/Node3D.hpp"
#include "Node/CameraNode.hpp"
#include "Math/Math.hpp"

//#include "Render/private/vulkan/Renderer/ForwardPass.hpp"
//#include "Render/private/vulkan/Renderer/DeferredPass.hpp"

#include "Template/Access.hpp"
#include "Template/Uninitialized.hpp"

#ifdef OJOIE_USE_GLFW
//#include <GLFW/glfw3.h>

#endif


#include <unordered_map>
#include <set>
#include <filesystem>
#include <fstream>
#include <variant>

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

struct RCTextureImplTag : Access::TagBase<RCTextureImplTag> {};

}

template struct Access::Accessor<DeviceImplTag, &RC::Device::impl>;
//template struct Access::Accessor<CommandQueueImplTag, &RC::CommandQueue::impl>;

template struct Access::Accessor<BufferPoolImplTag, &RC::BufferPool::impl>;
template struct Access::Accessor<BufferPoolShouldFreeTag, &RC::BufferPool::shouldFree>;

template struct Access::Accessor<BufferManagerImplTag, &RC::BufferManager::impl>;
template struct Access::Accessor<BufferManagerShouldFreeTag, &RC::BufferManager::shouldFree>;

template struct Access::Accessor<BlitCommandEncoderImplTag, &RC::BlitCommandEncoder::impl>;
template struct Access::Accessor<BlitCommandEncoderShouldFreeTag, &RC::BlitCommandEncoder::shouldFree>;

template struct Access::Accessor<RenderCommandEncoderImplTag, &RC::RenderCommandEncoder::impl>;

//template struct Access::Accessor<RCTextureImplTag, &RC::Texture::impl>;
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


#ifdef OJOIE_WITH_EDITOR

namespace {
struct RC_TextureImpl {
    Uninitialized<VK::Image> image;
    Uninitialized<VK::ImageView> imageView;
} viewportTextureImpl;
}

//static Uninitialized<RC::Texture> viewportTextureBridge = [] {
//    Uninitialized<RC::Texture> bridge{};
//    Access::set<RCTextureImplTag>(bridge.get(),
//                                  (Access::TagTrait<RCTextureImplTag>::ValueType)&viewportTextureImpl);
//    return bridge;
//}();

///// private export global function
//RC::Texture &__getViewportTexture() {
////    return viewportTextureBridge.get();
//}

static void SetViewportImageView(VK::ImageView &view) {
    viewportTextureImpl.imageView.construct(view.getImage().getDevice(),
                                            view.getImage(),
                                            view.vkImageView(),
                                            view.getFormat());
}

#endif //OJOIE_WITH_EDITOR


struct Renderer::Impl {

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VK::Instance instance;
    VK::Device device;

    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };

//    VK::CommandQueue commandQueue; // an commandQueue associated with primary queue

//    VK::CommandBuffer initCommandBuffer;

//    std::unordered_map<Window *, VK::Layer> layers;

    RenderContext *renderContext;

//    std::variant<std::monostate,
//                 VK::ForwardPass,
//                 VK::ForwardMSAAPass,
//                 VK::DeferredPass,
//                 VK::DeferredTAAPass> rendererPassImpl;

#ifdef OJOIE_WITH_EDITOR
    VK::Image editorViewportImage;
    VK::ImageView editorViewportImageView;
#endif

//    VK::RenderTarget layerCreateRenderTarget(uint32_t index, VK::Image &&swapchainImage) {
//
//#ifdef OJOIE_WITH_EDITOR
//        std::destroy_at(&editorViewportImage);
//        std::construct_at(&editorViewportImage);
//        std::destroy_at(&editorViewportImageView);
//        std::construct_at(&editorViewportImageView);
//        VK::ImageDescriptor imageDescriptor = VK::ImageDescriptor::Default2D();
//        imageDescriptor.extent = swapchainImage.getExtent();
//        imageDescriptor.format = VK_FORMAT_R8G8B8A8_UNORM;
//        imageDescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//        imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO;
//        imageDescriptor.allocationFlag = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
//
//        if (!editorViewportImage.init(device, imageDescriptor)) {
//            throw Exception("could not create editor viewport image");
//        }
//
//        if (!editorViewportImageView.init(editorViewportImage, VK_IMAGE_VIEW_TYPE_2D)) {
//            throw Exception("could not create editor viewport image view");
//        }
//
//#endif
//
//        return std::visit([&](auto &&pass) -> VK::RenderTarget {
//            using T = std::decay_t<decltype(pass)>;
//            if constexpr (std::is_same_v<T, std::monostate>) {
//                throw Exception("Renderer pass impl is invalid");
//            } else {
//                return pass.createRenderTarget(index, std::move(swapchainImage));
//            }
//
//        }, rendererPassImpl);
//    }

//
//    void renderPassConfigDidChange(const std::any&) {
//        auto task = [this] {
//            if (GetConfiguration().getObject<bool>("forward-shading")) {
//                renderContext->forwardShading = true;
//                if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "MSAA") == 0) {
//
//                    renderContext->msaaSamples = msaaSamples;
//                    renderContext->antiAliasing = kAntiAliasingMSAA;
//
//                    rendererPassImpl.emplace<VK::ForwardMSAAPass>();
//                    if (!std::get<VK::ForwardMSAAPass>(rendererPassImpl).init(device, renderContext->msaaSamples)) {
//                        throw Exception("fail to init VK::ForwardMSAAPass");
//                    }
//
//                } else {
//                    renderContext->msaaSamples = 1;
//
//                    rendererPassImpl.emplace<VK::ForwardPass>();
//                    if (!std::get<VK::ForwardPass>(rendererPassImpl).init(device)) {
//                        throw Exception("fail to init VK::ForwardPass");
//                    }
//                }
//
//            } else {
//                renderContext->forwardShading = false;
//                renderContext->msaaSamples = 1;
//
//                if (strcmp(GetConfiguration().getObject<const char *>("anti-aliasing"), "TAA") == 0) {
//                    renderContext->antiAliasing = kAntiAliasingTAA;
//                    rendererPassImpl.emplace<VK::DeferredTAAPass>();
//                    if (!std::get<VK::DeferredTAAPass>(rendererPassImpl).init(device)) {
//                        throw Exception("fail to init VK::DeferredTAAPass");
//                    }
//                } else {
//
//                    rendererPassImpl.emplace<VK::DeferredPass>();
//                    if (!std::get<VK::DeferredPass>(rendererPassImpl).init(device)) {
//                        throw Exception("fail to init VK::DeferredPass");
//                    }
//
//                }
//            }
//        };
//
//        if (GetCurrentThreadID() == Dispatch::GetThreadID(Dispatch::Render)) {
//            task();
//        } else if (GetCurrentThreadID() == Dispatch::GetThreadID(Dispatch::Game)) {
//            GetRenderQueue().enqueue(task);
//        } else {
//            Dispatch::async(Dispatch::Game, [=] {
//                GetRenderQueue().enqueue(task);
//            });
//        }
//
//    }
};

Renderer::Renderer() : impl(new Impl{}){
    renderContext.graphicContext = new GraphicContext{};
    impl->renderContext = &renderContext;
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
//    deviceDescriptor.requestedFeatures = deviceFeatures;
    deviceDescriptor.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkSurfaceKHR testSurface;

#ifdef OJOIE_USE_GLFW

//    if (glfwCreateWindowSurface(impl->instance.vKInstance(),
//                                (GLFWwindow *) currentWindow.load(std::memory_order_relaxed)->getUnderlyingWindow(),
//                                nullptr, &testSurface) != VK_SUCCESS) {
//        ANLog("failed to create window surface!");
//        return false;
//    }
#else

#error "not implement"

#endif

//    deviceDescriptor.surface = testSurface;
    deviceDescriptor.onlyOneDevice = true;

    if (!impl->device.init(deviceDescriptor)) {
        return false;
    }

    vkDestroySurfaceKHR(impl->instance.vKInstance(), testSurface, nullptr);

//    if (!impl->commandQueue.init(impl->device,
//                                 impl->device.queue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0),
//                                 kCommandBufferResetModeResetIndividually)) {
//        return false;
//    }

    renderContext.frameWidth = renderContext.windowWidth;
    renderContext.frameHeight = renderContext.windowHeight;

    renderContext.maxFrameInFlight = MAX_FRAMES_IN_FLIGHT;

    Access::set<DeviceImplTag>(renderContext.device, &impl->device);
    renderContext.graphicContext->device = &impl->device;

//    Access::set<CommandQueueImplTag>(renderContext.commandQueue, &impl->commandQueue);

    /// provide a blitCommandEncoder to init some render stuff
//    impl->initCommandBuffer = impl->commandQueue.commandBuffer();
//    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, new VK::BlitCommandEncoder(impl->initCommandBuffer.vkCommandBuffer()));
    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, true);

    VkBufferUsageFlagBits supportedUsageFlags[] = { VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
    VK::BufferManager *bufferManager = new VK::BufferManager();
    if (!bufferManager->init(impl->device, supportedUsageFlags, BUFFER_POOL_BLOCK_SIZE)) {
        return false;
    }

    Access::set<BufferManagerImplTag>(renderContext.bufferManager, bufferManager);
    Access::set<BufferManagerShouldFreeTag>(renderContext.bufferManager, true);

    impl->msaaSamples = getMaxUsableSampleCount(impl->device.getPhysicalDeviceProperties());

//    GetConfiguration().addObserver("forward-shading", impl, &Impl::renderPassConfigDidChange);
//    GetConfiguration().addObserver("anti-aliasing", impl, &Impl::renderPassConfigDidChange);
//    impl->renderPassConfigDidChange({});

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

    VK::GetRenderResourceCache().setPipelineCache(impl->pipelineCache);

    return true;
}


void Renderer::willDeinit() {
    /// called before resources deinit
    resourceFence();
    isStop = true;
}

void Renderer::resourceFence() {
    if (!isStop) {
        impl->device.getGraphicsQueue().waitIdle();
    }
}

void Renderer::deinit() {
    GetConfiguration().removeObserver(impl);

    impl->device.waitIdle();

#ifdef OJOIE_WITH_EDITOR
    impl->editorViewportImage.deinit();
    impl->editorViewportImageView.deinit();
#endif

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

//    std::visit([](auto &&pass) {
//        using T = std::decay_t<decltype(pass)>;
//        if constexpr (!std::is_same_v<T, std::monostate>) {
//            pass.deinit();
//        }
//
//    }, impl->rendererPassImpl);

    renderContext.bufferManager.deinit();
//    impl->layers.clear();

//    impl->commandQueue.deinit();
    impl->device.deinit();
    impl->instance.deinit();
}


void Renderer::render(RC::Scene &scene, float deltaTime, float elapsedTime) {
    static bool firstTime = true;
    if (firstTime) [[unlikely]] {
        /// submit the init procedure command
//        impl->initCommandBuffer.submit();
//        impl->initCommandBuffer.waitUntilCompleted();

//        renderContext.commandQueue.reset();



        firstTime = false;
    }

    static Window *lastWindow = nullptr;
//    if (lastWindow != currentWindow.load(std::memory_order_relaxed)) [[unlikely]] {
//        lastWindow = currentWindow.load(std::memory_order_relaxed);
//
//        auto layerIter = impl->layers.find(lastWindow);
//        if (!impl->layers.contains(lastWindow)) [[unlikely]] {
//            VK::Layer &layer = impl->layers[lastWindow];
//
//            if (!layer.init(impl->device, lastWindow)) {
//                impl->layers.erase(lastWindow);
//                return;
//            }
//
//            layer.layerCreateRenderTarget.bind(impl, &Impl::layerCreateRenderTarget);

//            layer.prepare();
//        }
//    }


    renderContext.deltaTime = deltaTime;
    renderContext.elapsedTime = elapsedTime;

//    renderContext.window = currentWindow;
//    renderContext.cursorState = currentCursorState;

    renderContext.scene = &scene;

//    VK::Layer &layer = impl->layers[lastWindow];

//    VK::Presentable currentPresentable = layer.nextPresentable();

    /// skip this frame
//    if (currentPresentable.swapchain == nullptr) {
//        return;
//    }

//    VK::CommandBuffer commandBuffer = layer.getActiveFrame().commandBuffer(impl->device.queue(VK_QUEUE_GRAPHICS_BIT, 0),
//                                                                           VK::CommandBufferResetMode::ResetPool,
//                                                                           VK_COMMAND_BUFFER_LEVEL_PRIMARY);
//
//    VK::CommandBuffer blitCommandBuffer = layer.getActiveFrame().commandBuffer(impl->device.queue(VK_QUEUE_GRAPHICS_BIT, 0),
//                                                                           VK::CommandBufferResetMode::ResetPool,
//                                                                               VK_COMMAND_BUFFER_LEVEL_PRIMARY);

//    renderContext.graphicContext->commandBuffer = commandBuffer.vkCommandBuffer();



//    renderContext.frameWidth = (float)layer.getActiveFrame().getRenderTarget().extent.width;
//    renderContext.frameHeight = (float)layer.getActiveFrame().getRenderTarget().extent.height;


//    VK::RenderCommandEncoder renderCommandEncoder(impl->device,
//                                                  commandBuffer.vkCommandBuffer(),
//                                                  layer.getActiveFrame().getDescriptorSetManager());

//    renderContext.graphicContext->renderCommandEncoder = &renderCommandEncoder;

//    Access::set<RenderCommandEncoderImplTag>(renderContext.renderCommandEncoder, &renderCommandEncoder);

    std::destroy_at(&renderContext.blitCommandEncoder);
    std::construct_at(&renderContext.blitCommandEncoder);

//    VK::BlitCommandEncoder blitCommandEncoder(blitCommandBuffer.vkCommandBuffer());

//    Access::set<BlitCommandEncoderImplTag>(renderContext.blitCommandEncoder, &blitCommandEncoder);
//    Access::set<BlitCommandEncoderShouldFreeTag>(renderContext.blitCommandEncoder, false);

    std::destroy_at(&renderContext.bufferManager);
    std::construct_at(&renderContext.bufferManager);

//    Access::set<BufferManagerImplTag>(renderContext.bufferManager, &layer.getActiveFrame().getBufferManager());
    Access::set<BufferManagerShouldFreeTag>(renderContext.bufferManager, false);

//    auto &renderTarget = layer.getActiveFrame().getRenderTarget();

//    std::visit([&](auto &&pass) {
//        using T = std::decay_t<decltype(pass)>;
//        if constexpr (std::is_same_v<T, VK::DeferredPass>) {
//            pass.beginGeometryPass(renderCommandEncoder, renderTarget);
//
//        } else if constexpr(std::is_same_v<T, VK::DeferredTAAPass>) {
//            pass.beginGeometryPass(renderCommandEncoder, layer.getActiveFrameIndex());
//        } else if constexpr(std::is_same_v<T, VK::ForwardMSAAPass>) {
//            pass.beginRenderPass(renderCommandEncoder, renderTarget, layer.getActiveFrameIndex());
//        } else {
//            if constexpr (!std::is_same_v<T, std::monostate>) {
//                pass.beginRenderPass(renderCommandEncoder, renderTarget);
//            }
//        }
//
//    }, impl->rendererPassImpl);


//    renderCommandEncoder.setViewport({ .originX = 0.f, .originY = 0.f,
//                                       .width = renderContext.frameWidth, .height = renderContext.frameHeight,
//                                       .znear = 0.f, .zfar = 1.f  });
//
//    renderCommandEncoder.setScissor({ .x = 0, .y = 0, .width = (int)renderContext.frameWidth, .height = (int)renderContext.frameHeight });

//    renderCommandEncoder.setCullMode(RC::CullMode::Back);

//    VK::BufferAllocation uniformAllocation = layer.getActiveFrame().getBufferManager().buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Light));

    /// TODO move light to Scene class
//    Light *uniform = (Light *)uniformAllocation.map();
//    uniform->lightPos = { 1000.2f, 30.0f, 2.0f, 0.f };
//    uniform->lightColor = { 0.980f, 0.976f, 0.902f, 0.01f };

//    renderCommandEncoder.bindUniformBuffer(1, uniformAllocation.getOffset(), sizeof(Light), uniformAllocation.getBuffer());

    scene.doRender(renderContext);

//    std::visit([&](auto &&pass) {
//        using T = std::decay_t<decltype(pass)>;
//        if constexpr (std::is_same_v<T, VK::DeferredPass> || std::is_same_v<T, VK::DeferredTAAPass>) {
//            pass.nextLightingPass(renderContext, renderCommandEncoder);
//
//            if constexpr (std::is_same_v<T, VK::DeferredTAAPass>) {
//                pass.nextTAAPass(renderContext, renderCommandEncoder, layer.framesInFlight(), renderTarget);
//            }
//        }
//
//    }, impl->rendererPassImpl);


//    renderCommandEncoder.debugLabelInsert("Post Rendering", { 0.5f, 1.f, 0.5f, 1.f});


#ifdef OJOIE_WITH_EDITOR
//    std::visit([&](auto &&pass) {
//        using T = std::decay_t<decltype(pass)>;
//        if constexpr (std::is_same_v<T, VK::DeferredTAAPass> || std::is_same_v<T, VK::ForwardMSAAPass>) {
//            pass.nextEditorPass(renderContext, renderCommandEncoder);
//            SetViewportImageView(pass.getFrameEditorViewportImageView());
//        }
//    }, impl->rendererPassImpl);

    int msaaSamples = renderContext.msaaSamples;
    renderContext.msaaSamples = 1;
#endif

    scene.doPostRender(renderContext);


#ifdef OJOIE_WITH_EDITOR
    renderContext.msaaSamples = msaaSamples;
#endif

//    std::visit([&](auto &&pass) {
//        using T = std::decay_t<decltype(pass)>;
//        if constexpr (!std::is_same_v<T, std::monostate>) {
//            pass.endRenderPass(renderCommandEncoder);
//        }
//    }, impl->rendererPassImpl);
//
//    blitCommandBuffer.submit();
//
//
//    commandBuffer.presentDrawable(currentPresentable);
//
//    commandBuffer.submit();


//    impl->commandQueue.reset(); // reset the commandQueue if frame makes any commandBuffer

    ++renderContext.frameCount;

    if (completionHandler) [[likely]] {
        completionHandler();
    }

}




}