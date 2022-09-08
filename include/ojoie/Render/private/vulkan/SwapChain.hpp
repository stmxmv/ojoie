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

    SwapChain *oldSwapChain;
    Device *device;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    uint32_t imageCount;
    VkSurfaceTransformFlagBitsKHR transform;
    VkPresentModeKHR presentMode;
    VkImageUsageFlags imageUsage;


    static Self Default() {
        return {
                .imageCount = 3,
                .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
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

    VkImageUsageFlags image_usage;

public:

    SwapChain() = default;

    SwapChain(SwapChain &&other) noexcept : _device(other._device), surface(other.surface), handle(other.handle),
                                            images(std::move(other.images)), extent(other.extent), format(other.format),
                                            image_count(other.image_count), transform(other.transform),
                                            present_mode(other.present_mode), image_usage(other.image_usage) {
        other.handle = VK_NULL_HANDLE;
    }

    ~SwapChain() {
        deinit();
    }


    bool init(const SwapChainDescriptor &swapChainDescriptor);

    bool init(SwapChain &oldSwapChain, const VkExtent2D &aExtent) {
        SwapChainDescriptor swapChainDescriptor;
        swapChainDescriptor.oldSwapChain = &oldSwapChain;
        swapChainDescriptor.extent = aExtent;
        swapChainDescriptor.imageUsage = oldSwapChain.image_usage;
        swapChainDescriptor.presentMode = oldSwapChain.present_mode;
        swapChainDescriptor.transform = oldSwapChain.transform;
        swapChainDescriptor.imageCount = oldSwapChain.image_count;
        swapChainDescriptor.surface = oldSwapChain.surface;
        swapChainDescriptor.device = oldSwapChain._device;

        return init(swapChainDescriptor);
    }

    bool init(SwapChain &oldSwapChain, uint32_t imageCount) {
        SwapChainDescriptor swapChainDescriptor;
        swapChainDescriptor.oldSwapChain = &oldSwapChain;
        swapChainDescriptor.extent = extent;
        swapChainDescriptor.imageUsage = oldSwapChain.image_usage;
        swapChainDescriptor.presentMode = oldSwapChain.present_mode;
        swapChainDescriptor.transform = oldSwapChain.transform;
        swapChainDescriptor.imageCount = imageCount;
        swapChainDescriptor.surface = oldSwapChain.surface;
        swapChainDescriptor.device = oldSwapChain._device;

        return init(swapChainDescriptor);
    }

    bool init(SwapChain &oldSwapChain, VkImageUsageFlagBits image_usage_flags) {
        SwapChainDescriptor swapChainDescriptor;
        swapChainDescriptor.oldSwapChain = &oldSwapChain;
        swapChainDescriptor.extent = oldSwapChain.extent;
        swapChainDescriptor.imageUsage = image_usage_flags;
        swapChainDescriptor.presentMode = oldSwapChain.present_mode;
        swapChainDescriptor.transform = oldSwapChain.transform;
        swapChainDescriptor.imageCount = oldSwapChain.image_count;
        swapChainDescriptor.surface = oldSwapChain.surface;
        swapChainDescriptor.device = oldSwapChain._device;

        return init(swapChainDescriptor);
    }

    bool init(SwapChain &oldSwapChain, const VkExtent2D &aExtent, const VkSurfaceTransformFlagBitsKHR aTransform) {
        SwapChainDescriptor swapChainDescriptor;
        swapChainDescriptor.oldSwapChain = &oldSwapChain;
        swapChainDescriptor.extent = aExtent;
        swapChainDescriptor.imageUsage = oldSwapChain.image_usage;
        swapChainDescriptor.presentMode = oldSwapChain.present_mode;
        swapChainDescriptor.transform = aTransform;
        swapChainDescriptor.imageCount = oldSwapChain.image_count;
        swapChainDescriptor.surface = oldSwapChain.surface;
        swapChainDescriptor.device = oldSwapChain._device;

        return init(swapChainDescriptor);
    }

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

    VkImageUsageFlags getUsage() const {
        return image_usage;
    }
};



}

#endif//OJOIE_SWAPCHAIN_HPP
