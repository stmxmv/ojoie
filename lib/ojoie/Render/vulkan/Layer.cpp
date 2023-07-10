//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/Layer.hpp"
#include "BaseClasses/ObjectPtr.hpp"
#include "Core/CGTypes.hpp"
#include "Render/Layer.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {


bool Win32Layer::init(Window *window) {
    return init(GetDevice(), (WIN::Window *)window);
}

bool Win32Layer::init(Device &device, WIN::Window *window) {
    _device = &device;
    _window = window;

    if (!window) return false;

    _dpiScaleX = window->getDPIScaleX();
    _dpiScaleY = window->getDPIScaleY();

    _queue = &device.getGraphicsQueue();

    _presentable._renderTarget = AN::MakeObjectPtr<AN::RenderTarget>();

    ANAssert(_presentable._renderTarget->init()); // init empty renderTarget to bridge swapchain images

    /// create surface
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.hinstance = GetModuleHandleW(nullptr);
    surfaceCreateInfo.hwnd      = window->getHWND();

    VkResult result = vkCreateWin32SurfaceKHR(GetDevice().vkInstance(), &surfaceCreateInfo, nullptr, &surface);

    if (result != VK_SUCCESS) {
        ANLog("fail to create win32 vulkan surface, %s", ResultCString(result));
        return false;
    }

    Size rect = getSize();

    SwapChainDescriptor swapChainDescriptor = SwapChainDescriptor::Default();
    swapChainDescriptor.device              = &device;
    swapChainDescriptor.surface             = surface;
    swapChainDescriptor.extent.width        = rect.width;
    swapChainDescriptor.extent.height       = rect.height;

    if (!swapchain.init(swapChainDescriptor)) {
        return false;
    }

    if (swapchain.getImages().size() > kMaxFrameInFlight) {
        AN_LOG(Error, "swapchain max frame in flight value %zu exceeds maximum value", swapchain.getImages().size());
        return false;
    }

    surface_extent = swapchain.getExtent();

    prepare();

    return true;
}

void Win32Layer::deinit() {
    if (surface != VK_NULL_HANDLE) {
        swapchain.deinit();

        vkDestroySurfaceKHR(_device->vkInstance(), surface, nullptr);
        surface = VK_NULL_HANDLE;

        _presentable._renderTarget.reset();
    }
}

void Win32Layer::prepare() {
    _device->waitIdle();
    // If surface exists, create our RenderFrames from the swapchain (one for each image)
    if (surface) {
        for (uint32_t i = 0; i < swapchain.getImages().size(); ++i) {
            frames.emplace_back();
            ANAssert(frames.back().init(*_device));
            _renderTargets.emplace_back();
            _renderTargets.back().bridgeSwapchainRenderTarget(swapchain.getImages()[i],
                                                              swapchain.getExtent(),
                                                              swapchain.getFormat(),
                                                              0,
                                                              GetDevice().getSwapchainImageUsageFlags());
        }

    } else {
        // Otherwise, create a single RenderFrame
        /// TODO
        ANAssert(false && "not implement");
    }

    this->prepared = true;
}

bool Win32Layer::handle_surface_changes() {
    if (!surface) {
        ANLog("Can't handle surface changes in headless mode, skipping.");
        return false;
    }

    Size rect = getSize();

    if (rect.width == 0 || rect.height == 0) {
        return false;
    }

    _device->waitIdle();

    updateSwapchain({ .width = rect.width, .height = rect.height });
    surface_extent.width  = rect.width;
    surface_extent.height = rect.height;

//
//    if (rect.width != surface_extent.width ||
//        rect.height != surface_extent.height) {
//        // Recreate swapchain
//
//        _device->waitIdle();
//
//        do {
//            cpu_relax();
//            cpu_relax();
//
//            rect = getSize();
//
//        } while (rect.width == 0 || rect.height == 0);
//
//        updateSwapchain({ .width = rect.width, .height = rect.height });
//
//        surface_extent.width = rect.width;
//        surface_extent.height = rect.height;
//    }

    return true;
}

void Win32Layer::recreate() {
    _renderTargets.clear();
    for (uint32_t i = 0; i < swapchain.getImages().size(); ++i) {
        _renderTargets.emplace_back();
        _renderTargets.back().bridgeSwapchainRenderTarget(swapchain.getImages()[i],
                                                          swapchain.getExtent(),
                                                          swapchain.getFormat(),
                                                          0,
                                                          GetDevice().getSwapchainImageUsageFlags());
    }
}

void Win32Layer::updateSwapchain(const VkExtent2D &extent) {

    if (!surface) {
        ANLog("Can't update the swapchains extent and surface transform in headless mode, skipping.");
        return;
    }

    auto width  = extent.width;
    auto height = extent.height;

    SwapChain newSwapChain{};

    if (newSwapChain.init(swapchain, VkExtent2D{width, height})) {
        swapchain.deinit();
        std::destroy_at(&swapchain);

        std::construct_at(&swapchain, std::move(newSwapChain));

        AN_LOG(Debug, "recreate vulkan swapchain");
    } else {
        return;
    }


    recreate();
}


Presentable *Win32Layer::nextPresentable() {
//    // Only handle surface changes if a swapchain exists
//    if (surface) {
//        if (!handle_surface_changes()) {
//            return nullptr;
//        }
//    }

    RenderFrame &prev_frame = frames.at(active_frame_index);

    VkSemaphore acquired_semaphore = prev_frame.semaphore();

    if (acquired_semaphore == VK_NULL_HANDLE) {
        return nullptr;
    }

    if (surface) {
        VkResult result = swapchain.acquireNextImage(active_frame_index, acquired_semaphore, nullptr);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            if (handle_surface_changes()) {
                result = swapchain.acquireNextImage(active_frame_index, acquired_semaphore, nullptr);
            } else {
                result = VK_NOT_READY;
            }
        }

        if (result != VK_SUCCESS) {
            prev_frame.reset();

            return nullptr;
        }
    }

    /// wait current active frame to complete its last drawing
    waitFrame();


    _presentable.signalSemaphore = getActiveFrame().semaphore();
    _presentable.acquireSemaphore = acquired_semaphore;
    _presentable.waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    _presentable.swapchain = swapchain.vkSwapchainKHR();
    _presentable.imageIndex = active_frame_index;

    _presentable._renderTarget->bridgeSwapchinRenderTargetInternal(&_renderTargets[active_frame_index]);

    return &_presentable;
}

Size Win32Layer::getSize() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->vkPhysicalDevice(), surface, &surfaceCapabilities)) {
        AN_LOG(Error, "vulkan surface get capabilities fail");
    }

    return { .width = surfaceCapabilities.currentExtent.width,
             .height = surfaceCapabilities.currentExtent.height };
}

}
// namespace AN::VK