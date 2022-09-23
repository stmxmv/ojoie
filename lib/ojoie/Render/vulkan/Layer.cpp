//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Layer.hpp"

namespace AN::VK {


bool Layer::init(Device &device, Window *window) {
    _device = &device;
    _window = window;

    _queue = &device.graphicsQueue();

    if (window) {
        /// create surface
#ifdef OJOIE_USE_GLFW

        if (glfwCreateWindowSurface(device.vkInstance(), (GLFWwindow *) window->getUnderlyingWindow(), nullptr, &surface) != VK_SUCCESS) {
            ANLog("failed to create window surface!");
            return false;
        }

#endif

        VkSurfaceCapabilitiesKHR surface_properties;
        if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->vkPhysicalDevice(), surface, &surface_properties);
            result != VK_SUCCESS) {

            ANLog("vkGetPhysicalDeviceSurfaceCapabilitiesKHR return %s", ResultCString(result));

            return false;
        }

        SwapChainDescriptor swapChainDescriptor = SwapChainDescriptor::Default();
        swapChainDescriptor.device              = &device;
        swapChainDescriptor.surface             = surface;
        swapChainDescriptor.extent = surface_properties.currentExtent;

        if (!swapchain.init(swapChainDescriptor)) {
            return false;
        }

        surface_extent = swapchain.getExtent();

    } else {

        surface = VK_NULL_HANDLE;
    }


    return true;
}
void Layer::deinit() {
    if (surface != VK_NULL_HANDLE) {
        swapchain.deinit();

        vkDestroySurfaceKHR(_device->vkInstance(), surface, nullptr);

        surface = VK_NULL_HANDLE;
    }
}
void Layer::prepare() {
    _device->waitIdle();
    // If surface exists, create our RenderFrames from the swapchain (one for each image)
    if (surface) {
        VkExtent3D extent{surface_extent.width, surface_extent.height, 1};

        for (uint32_t i = 0; i < swapchain.getImages().size(); ++i) {
            Image swapchainImage;
            swapchainImage.init(*_device, swapchain.getImages()[i], extent, swapchain.getFormat(), swapchain.getUsage());

            auto render_target = layerCreateRenderTarget(i, std::move(swapchainImage));

            RenderFrame renderFrame;
            renderFrame.init(*_device, std::move(render_target));

            frames.emplace_back(std::move(renderFrame));
        }
    } else {
        // Otherwise, create a single RenderFrame
        /// TODO
    }

    this->prepared = true;
}

bool Layer::handle_surface_changes() {
    if (!surface) {
        ANLog("Can't handle surface changes in headless mode, skipping.");
        return true;
    }

    VkSurfaceCapabilitiesKHR surface_properties;
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->vkPhysicalDevice(), surface, &surface_properties);
        result != VK_SUCCESS) {

        ANLog("vkGetPhysicalDeviceSurfaceCapabilitiesKHR return %s", ResultCString(result));

        return false;
    }

    if (surface_properties.currentExtent.width != surface_extent.width ||
        surface_properties.currentExtent.height != surface_extent.height) {
        // Recreate swapchain

        _device->waitIdle();

        do {
            cpu_relax();
            cpu_relax();

            if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->vkPhysicalDevice(), surface, &surface_properties);
                result != VK_SUCCESS) {

                ANLog("vkGetPhysicalDeviceSurfaceCapabilitiesKHR return %s", ResultCString(result));

                return false;
            }

        } while (surface_properties.currentExtent.width == 0 || surface_properties.currentExtent.height == 0);

        updateSwapchain(surface_properties.currentExtent, pre_transform);

        surface_extent = surface_properties.currentExtent;
    }

    return true;
}

void Layer::recreate() {
    VkExtent2D swapchain_extent = swapchain.getExtent();
    VkExtent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

    auto frame_it = frames.begin();

    for (uint32_t i = 0; i < swapchain.getImages().size(); ++i) {
        Image swapchainImage;
        swapchainImage.init(*_device, swapchain.getImages()[i], extent, swapchain.getFormat(), swapchain.getUsage());

        auto render_target = layerCreateRenderTarget(i, std::move(swapchainImage));

        frame_it->replaceRenderTarget(std::move(render_target));

        ++frame_it;
    }
}

void Layer::updateSwapchain(const VkExtent2D &extent, VkSurfaceTransformFlagBitsKHR transform) {

    if (!surface) {
        ANLog("Can't update the swapchains extent and surface transform in headless mode, skipping.");
        return;
    }

    /// just clear any frame buffers
    /// TODO  clear layer's specific frame buffers
    _device->getRenderResourceCache().clearFrameBuffers();

    auto width  = extent.width;
    auto height = extent.height;
    if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        // Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
        std::swap(width, height);
    }


    SwapChain newSwapChain{};

    if (newSwapChain.init(swapchain, VkExtent2D{width, height}, transform)) {
        swapchain.deinit();
        std::destroy_at(&swapchain);

        std::construct_at(&swapchain, std::move(newSwapChain));
    } else {
        return;
    }

    // Save the preTransform attribute for future rotations
    pre_transform = transform;

    recreate();
}


Presentable Layer::nextPresentable() {
    // Only handle surface changes if a swapchain exists
    if (surface) {
        if (!handle_surface_changes()) {
            return {};
        }
    }

    RenderFrame &prev_frame = frames.at(active_frame_index);

    VkSemaphore acquired_semaphore = prev_frame.semaphore();

    if (acquired_semaphore == VK_NULL_HANDLE) {
        return {};
    }

    if (surface) {
        auto *fence = prev_frame.fence();

        auto result = swapchain.acquireNextImage(active_frame_index, acquired_semaphore, fence);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            handle_surface_changes();

            result = swapchain.acquireNextImage(active_frame_index, acquired_semaphore, fence);
        }

        if (result != VK_SUCCESS) {
            prev_frame.reset();

            return {};
        }
    }

    /// wait current active frame to complete its last drawing
    waitFrame();

    getActiveFrame().getDescriptorSetManager().clearFrameSets();

    return {
            .signalSemaphore = getActiveFrame().semaphore(),
            .acquireSemaphore = acquired_semaphore,
            .waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .swapchain = swapchain.vkSwapchainKHR(),
            .imageIndex = active_frame_index
    };
}

}