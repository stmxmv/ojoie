//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_SWAPCHAIN_HPP
#define OJOIE_SWAPCHAIN_HPP

#include "Render/private/vulkan/Image.hpp"

namespace AN::VK {

class SwapChain;

struct SwapChainDescriptor {
    typedef SwapChainDescriptor Self;

    SwapChain                    *oldSwapChain;
    Device                       *device;
    VkSurfaceKHR                  surface;
    VkExtent2D                    extent;
    uint32_t                      imageCount;
    VkSurfaceTransformFlagBitsKHR transform;
    VkPresentModeKHR              presentMode;


    static Self Default() {
        return {
            .imageCount  = 3,
            .transform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_MAILBOX_KHR
        };
    }
};

class SwapChain {
    Device *_device;

    VkSurfaceKHR surface;

    VkSwapchainKHR handle{};

    std::vector<VkImage> images;

    VkExtent2D extent;

    VkFormat format;

    uint32_t image_count;

    VkSurfaceTransformFlagBitsKHR transform;

    VkPresentModeKHR present_mode;

public:
    SwapChain() = default;

    SwapChain(SwapChain &&other) noexcept;

    ~SwapChain() {
        deinit();
    }


    bool init(const SwapChainDescriptor &swapChainDescriptor);

    bool init(SwapChain &oldSwapChain, const VkExtent2D &aExtent);

    void deinit();

    VkSwapchainKHR vkSwapchainKHR() const {
        return handle;
    }

    VkResult acquireNextImage(uint32_t &image_index, VkSemaphore image_acquired_semaphore, VkFence fence);

    const std::vector<VkImage> &getImages() const {
        return images;
    }

    const VkExtent2D &getExtent() const {
        return extent;
    }

    VkFormat getFormat() const {
        return format;
    }
};


}// namespace AN::VK

#endif//OJOIE_SWAPCHAIN_HPP
