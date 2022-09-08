//
// Created by Aleudillonam on 9/2/2022.
//

#ifndef OJOIE_LAYER_HPP
#define OJOIE_LAYER_HPP

#include "GLFW/glfw3.h"
#include "Render/private/vulkan/SwapChain.hpp"
#include "Render/private/vulkan/FrameBuffer.hpp"
#include "Render/private/vulkan/RenderPass.hpp"
#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/RenderFrame.hpp"

#include "Core/Window.hpp"


namespace AN::VK {


class Layer {

    Window *_window;

    Device *_device;

    VkSurfaceKHR surface{};

    /// If swapchain exists, then this will be a present supported queue, else a graphics queue
    const Queue *_queue;

    SwapChain swapchain;

    std::vector<RenderFrame> frames;

    VkSemaphore acquired_semaphore;
    VkSemaphore render_semaphore;

    bool prepared{};

    /// Whether a frame is active or not
    bool frame_active{};

    /// Current active frame index
    uint32_t active_frame_index{};

    VkSurfaceTransformFlagBitsKHR pre_transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};


    VkSemaphore _beginFrame();

    VkSemaphore _submit(const Queue &queue, VkCommandBuffer &command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage);

    void _submit(const Queue &queue, VkCommandBuffer command_buffer);

protected:
    VkExtent2D surface_extent;

    /**
	 * @brief Handles surface changes, only applicable if the render_context makes use of a swapchain
	 */
    bool handle_surface_changes();

public:

    Layer() = default;

    Layer(Layer &&other) noexcept
        : _window(other._window), _device(other._device), surface(other.surface), _queue(other._queue),
          swapchain(std::move(other.swapchain)), frames(std::move(other.frames)),
          acquired_semaphore(other.acquired_semaphore), render_semaphore(other.render_semaphore),
          prepared(other.prepared), frame_active(other.frame_active), active_frame_index(other.active_frame_index),
          pre_transform(other.pre_transform), surface_extent(other.surface_extent) {

        other._window = nullptr;
        other._device = nullptr;
        other.surface = VK_NULL_HANDLE;
        other._queue = nullptr;
        other.acquired_semaphore = VK_NULL_HANDLE;
        other.render_semaphore = VK_NULL_HANDLE;
    }

    ~Layer() {
        deinit();
    }

    bool init(Device &device, Window *window);

    void deinit();

    void prepare();

    void updateSwapchain(const VkExtent2D &extent, VkSurfaceTransformFlagBitsKHR transform);

    void recreate();

    RenderFrame &getActiveFrame() {
        assert(frame_active && "Frame is not active, please call _beginFrame");
        return frames.at(active_frame_index);
    }

    uint32_t getActiveFrameIndex() const {
        assert(frame_active && "Frame is not active, please call _beginFrame");
        return active_frame_index;
    }

    VkCommandBuffer beginFrame(CommandBufferResetMode reset_mode = CommandBufferResetMode::ResetPool);

    void submit(VkCommandBuffer command_buffer);

    void present();

    void waitFrame() {
        getActiveFrame().reset();
    }

    Device &getDevice() const {
        return *_device;
    }

    /// \required
    Delegate<RenderTarget(Image &&)> layerCreateRenderTarget;


};



}

#endif//OJOIE_LAYER_HPP
