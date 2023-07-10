//
// Created by Aleudillonam on 9/2/2022.
//

#ifndef OJOIE_VK_LAYER_HPP
#define OJOIE_VK_LAYER_HPP


#include "ojoie/Render/private/vulkan/SwapChain.hpp"
#include "ojoie/Render/private/vulkan/FrameBuffer.hpp"
#include "ojoie/Render/private/vulkan/RenderPass.hpp"
#include "ojoie/Render/private/vulkan/Queue.hpp"
#include "ojoie/Render/private/vulkan/RenderFrame.hpp"
#include <ojoie/Render/private/vulkan/Presentable.hpp>
#include <ojoie/Template/SmallVector.hpp>
#include <ojoie/Render/Layer.hpp>
#include <ojoie/Template/delegate.hpp>

#include <ojoie/Core/private/win32/Window.hpp>

#include <Windows.h>

namespace AN::VK {


/// layer is the abstraction of swapchain images and per-frame data
class Win32Layer : public AN::Layer {

    float _dpiScaleX, _dpiScaleY;
    WIN::Window *_window;

    Device *_device;

    VkSurfaceKHR surface{};

    /// If swapchain exists, then this will be a present supported queue, else a graphics queue
    const Queue *_queue;

    SwapChain swapchain;

    SmallVector<RenderFrame, kMaxFrameInFlight> frames;

    bool prepared{};

    /// Whether a frame is active or not
    bool frame_active{};

    /// Current active frame index
    uint32_t active_frame_index{};

    /// the screen render target
    SmallVector<RenderTarget, kMaxFrameInFlight> _renderTargets;

    Presentable _presentable;

protected:
    VkExtent2D surface_extent;

    /**
	 * @brief Handles surface changes, only applicable if the render_context makes use of a swapchain
	 */
    bool handle_surface_changes();

    void prepare();

public:

    Win32Layer() = default;

    virtual ~Win32Layer() override {
        deinit();
    }

    bool init(Device &device, WIN::Window *window);

    virtual bool init(Window *window) override;

    void deinit();

    void updateSwapchain(const VkExtent2D &extent);

    void recreate();

    RenderFrame &getActiveFrame() {
        return frames.at(active_frame_index);
    }

    uint32_t framesInFlight() const {
        return swapchain.getImages().size();
    }

    uint32_t getActiveFrameIndex() const {
        return active_frame_index;
    }

    virtual Presentable *nextPresentable() override;

    virtual Size getSize() override;

    virtual void getDpiScale(float &x, float &y) override {
        x = _dpiScaleX;
        y = _dpiScaleY;
    }

    virtual Window *getWindow() override { return _window; }

    void waitFrame() {
        getActiveFrame().reset();
    }

    Device &getDevice() const {
        return *_device;
    }

    virtual void resize(const Size &size) override {
        /// TODO
    }
};



}

#endif//OJOIE_VK_LAYER_HPP
