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

        for (const auto &image_handle : swapchain.getImages()) {
            Image swapchainImage;
            swapchainImage.init(*_device, image_handle, extent, swapchain.getFormat(), swapchain.getUsage());

            auto render_target = layerCreateRenderTarget(std::move(swapchainImage));

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

    for (const auto &image_handle : swapchain.getImages()) {
        Image swapchainImage;
        swapchainImage.init(*_device, image_handle, extent, swapchain.getFormat(), swapchain.getUsage());

        auto render_target = layerCreateRenderTarget(std::move(swapchainImage));

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

VkSemaphore Layer::_beginFrame() {
    // Only handle surface changes if a swapchain exists
    if (surface) {
        if (!handle_surface_changes()) {
            return nullptr;
        }
    }

    assert(!frame_active && "Frame is still active, please call end_frame");

    auto &prev_frame = frames.at(active_frame_index);

    auto *aquired_semaphore = prev_frame.semaphore();

    if (surface) {
        auto *fence = prev_frame.fence();

        auto result = swapchain.acquireNextImage(active_frame_index, aquired_semaphore, fence);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            handle_surface_changes();

            result = swapchain.acquireNextImage(active_frame_index, aquired_semaphore, fence);
        }

        if (result != VK_SUCCESS)
        {
            prev_frame.reset();

            return VK_NULL_HANDLE;
        }
    }

    // Now the frame is active again
    frame_active = true;

    waitFrame();

    return aquired_semaphore;
}

VkCommandBuffer Layer::beginFrame(CommandBufferResetMode reset_mode) {

    assert(prepared && "RenderContext not prepared for rendering, call prepare()");

    acquired_semaphore = _beginFrame();

    if (acquired_semaphore == VK_NULL_HANDLE) {
        return nullptr;
//        throw std::runtime_error("Couldn't begin frame");
    }

    const auto &queue = _device->queue(VK_QUEUE_GRAPHICS_BIT, 0);
    return getActiveFrame().commandBuffer(queue, reset_mode);
}


void Layer::_submit(const Queue &queue, VkCommandBuffer command_buffer) {
    RenderFrame &frame = getActiveFrame();

    VkCommandBuffer cmd_buf = command_buffer;

    VkSubmitInfo submit_info[1] = {};
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers    = &cmd_buf;

    VkFence fence = frame.fence();

    queue.submit(submit_info, fence);
}


VkSemaphore Layer::_submit(const Queue &queue, VkCommandBuffer &command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage) {
    RenderFrame &frame = getActiveFrame();

    VkSemaphore signal_semaphore = frame.semaphore();

    VkCommandBuffer cmd_buf = command_buffer;

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};

    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd_buf;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &wait_semaphore;
    submit_info.pWaitDstStageMask    = &wait_pipeline_stage;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &signal_semaphore;

    VkFence fence = frame.fence();

    VkSubmitInfo infos[] = { submit_info };
    queue.submit(infos, fence);

    return signal_semaphore;
}


void Layer::submit(VkCommandBuffer command_buffer) {
    assert(frame_active && "RenderContext is inactive, cannot submit command buffer. Please call begin()");

    render_semaphore = VK_NULL_HANDLE;

    if (surface) {
        render_semaphore = _submit(*_queue, command_buffer, acquired_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    } else {
        _submit(*_queue, command_buffer);
    }

    acquired_semaphore = VK_NULL_HANDLE;
}

void Layer::present() {
    assert(frame_active && "Frame is not active, please call begin_frame");

    if (surface) {
        VkSwapchainKHR vk_swapchain = swapchain.vkSwapchainKHR();

        VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &render_semaphore;
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &vk_swapchain;
        present_info.pImageIndices      = &active_frame_index;

        VkResult result = _queue->present(present_info);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            handle_surface_changes();
        }
    }

    // Frame is not active anymore
    frame_active = false;
}

}