//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/SwapChain.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {


namespace detail {

inline uint32_t choose_image_count(uint32_t request_image_count, uint32_t min_image_count, uint32_t max_image_count) {
    request_image_count = std::min(request_image_count, max_image_count);
    request_image_count = std::max(request_image_count, min_image_count);

    return request_image_count;
}


inline VkExtent2D choose_extent(
        VkExtent2D        request_extent,
        const VkExtent2D &min_image_extent,
        const VkExtent2D &max_image_extent,
        const VkExtent2D &current_extent) {
    if (request_extent.width < 1 || request_extent.height < 1) {
        ANLog("Requested image extent not supported. Selected default value.");

        return current_extent;
    }

    request_extent.width = std::max(request_extent.width, min_image_extent.width);
    request_extent.width = std::min(request_extent.width, max_image_extent.width);

    request_extent.height = std::max(request_extent.height, min_image_extent.height);
    request_extent.height = std::min(request_extent.height, max_image_extent.height);

    return request_extent;
}

template<typename VkPresentModeKHRs>
inline VkPresentModeKHR choose_present_mode(
        VkPresentModeKHR    request_present_mode,
        VkPresentModeKHRs &&present_modes) {
    auto present_mode_it = std::find(present_modes.begin(), present_modes.end(), request_present_mode);

    if (present_mode_it == std::end(present_modes)) {
        present_mode_it = std::begin(present_modes);

        ANLog("Requested present mode not supported. Selected default value.");
    }

    return *present_mode_it;
}

inline VkSurfaceTransformFlagBitsKHR choose_transform(
        VkSurfaceTransformFlagBitsKHR request_transform,
        VkSurfaceTransformFlagsKHR    supported_transform,
        VkSurfaceTransformFlagBitsKHR current_transform) {
    if (request_transform & supported_transform) {
        return request_transform;
    }

    ANLog("Requested transform not supported. Selected default value.");

    return current_transform;
}

inline VkCompositeAlphaFlagBitsKHR choose_composite_alpha(
        VkCompositeAlphaFlagBitsKHR request_composite_alpha,
        VkCompositeAlphaFlagsKHR    supported_composite_alpha) {

    if (request_composite_alpha & supported_composite_alpha) {
        return request_composite_alpha;
    }

    ANLog("Requested composite alpha not supported. Selected default value.");

    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

    for (VkCompositeAlphaFlagBitsKHR composite_alpha : composite_alpha_flags) {
        if (composite_alpha & supported_composite_alpha) {
            return composite_alpha;
        }
    }

    throw std::runtime_error("No compatible composite alpha found.");
}


}// namespace detail

SwapChain::SwapChain(SwapChain &&other) noexcept
    : _device(other._device), surface(other.surface), handle(other.handle),
      images(std::move(other.images)), extent(other.extent), format(other.format),
      image_count(other.image_count), transform(other.transform),
      present_mode(other.present_mode) {
    other.handle = VK_NULL_HANDLE;
}

bool SwapChain::init(const SwapChainDescriptor &swapChainDescriptor) {
    _device      = swapChainDescriptor.device;
    surface      = swapChainDescriptor.surface;
    image_count  = swapChainDescriptor.imageCount;
    transform    = swapChainDescriptor.transform;
    present_mode = swapChainDescriptor.presentMode;


#ifdef VK_CHECK
#undef VK_CHECK
#endif
#define VK_CHECK(x)                              \
    do {                                         \
        VkResult err = x;                        \
        if (err) {                               \
            ANLog("Detected Vulkan error: " #x); \
            return false;                        \
        }                                        \
    } while (0)

    uint32_t present_mode_count{0U};
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(_device->vkPhysicalDevice(), surface, &present_mode_count, nullptr));

    std::vector<VkPresentModeKHR> present_modes{present_mode_count};
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(_device->vkPhysicalDevice(), surface, &present_mode_count,
                                                       present_modes.data()));

    VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};


    const VkSurfaceCapabilitiesKHR &deviceSurfaceCapabilities = GetDevice().getSurfaceCpabilities();

    create_info.minImageCount = detail::choose_image_count(swapChainDescriptor.imageCount,
                                                           deviceSurfaceCapabilities.minImageCount,
                                                           deviceSurfaceCapabilities.maxImageCount);

    create_info.imageExtent = swapChainDescriptor.extent;


    create_info.imageFormat     = GetDevice().getVkSurfaceFormatKHR().format;
    create_info.imageColorSpace = GetDevice().getVkSurfaceFormatKHR().colorSpace;

    create_info.imageArrayLayers = 1U;
    create_info.imageUsage       = GetDevice().getSwapchainImageUsageFlags();
    /// we don't use any flags
    create_info.flags = 0;

    create_info.preTransform   = detail::choose_transform(swapChainDescriptor.transform,
                                                          deviceSurfaceCapabilities.supportedTransforms,
                                                          deviceSurfaceCapabilities.currentTransform);
    create_info.compositeAlpha = detail::choose_composite_alpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                                deviceSurfaceCapabilities.supportedCompositeAlpha);
    create_info.presentMode    = detail::choose_present_mode(swapChainDescriptor.presentMode, present_modes);

    create_info.surface      = swapChainDescriptor.surface;
    create_info.oldSwapchain = swapChainDescriptor.oldSwapChain ? swapChainDescriptor.oldSwapChain->handle : VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(_device->vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create SwapChain %s", ResultCString(result));
        return false;
    }

    uint32_t image_available{0u};
    VK_CHECK(vkGetSwapchainImagesKHR(_device->vkDevice(), handle, &image_available, nullptr));

    images.resize(image_available);

    VK_CHECK(vkGetSwapchainImagesKHR(_device->vkDevice(), handle, &image_available, images.data()));

    this->extent       = create_info.imageExtent;
    this->image_count  = create_info.minImageCount;
    this->transform    = create_info.preTransform;
    this->present_mode = create_info.presentMode;
    this->format       = create_info.imageFormat;


#undef VK_CHECK
    return true;
}

bool SwapChain::init(SwapChain &oldSwapChain, const VkExtent2D &aExtent) {
    SwapChainDescriptor swapChainDescriptor{};
    swapChainDescriptor.oldSwapChain = &oldSwapChain;
    swapChainDescriptor.presentMode  = oldSwapChain.present_mode;
    swapChainDescriptor.transform    = oldSwapChain.transform;
    swapChainDescriptor.imageCount   = oldSwapChain.image_count;
    swapChainDescriptor.surface      = oldSwapChain.surface;
    swapChainDescriptor.device       = oldSwapChain._device;
    swapChainDescriptor.extent       = aExtent;

    return init(swapChainDescriptor);
}

void SwapChain::deinit() {
    if (handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
VkResult SwapChain::acquireNextImage(uint32_t &image_index, VkSemaphore image_acquired_semaphore, VkFence fence) {
    return vkAcquireNextImageKHR(_device->vkDevice(), handle, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence, &image_index);
}


}// namespace AN::VK