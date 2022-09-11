//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/SwapChain.hpp"

namespace AN::VK {


namespace detail {

inline uint32_t choose_image_count(uint32_t request_image_count,uint32_t min_image_count,uint32_t max_image_count) {
    request_image_count = std::min(request_image_count, max_image_count);
    request_image_count = std::max(request_image_count, min_image_count);

    return request_image_count;
}


inline uint32_t choose_image_array_layers(
        uint32_t request_image_array_layers,
        uint32_t max_image_array_layers) {
    request_image_array_layers = std::min(request_image_array_layers, max_image_array_layers);
    request_image_array_layers = std::max(request_image_array_layers, 1u);

    return request_image_array_layers;
}

inline VkExtent2D choose_extent(
        VkExtent2D        request_extent,
        const VkExtent2D &min_image_extent,
        const VkExtent2D &max_image_extent,
        const VkExtent2D &current_extent
) {
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
        VkPresentModeKHR                     request_present_mode,
        VkPresentModeKHRs &&present_modes
) {
    auto present_mode_it = std::find(present_modes.begin(), present_modes.end(), request_present_mode);

    if (present_mode_it == std::end(present_modes)) {
        present_mode_it = std::begin(present_modes);

        ANLog("Requested present mode not supported. Selected default value.");
    }

    return *present_mode_it;
}

template<typename VkSurfaceFormatKHRs0, typename VkSurfaceFormatKHRs1>
inline VkSurfaceFormatKHR choose_surface_format(
        VkSurfaceFormatKHRs0 &&request_surface_formats,
        VkSurfaceFormatKHRs1 &&surface_formats) {

    decltype(std::find_if(std::begin(surface_formats),
                          std::end(surface_formats), VkSurfaceFormatKHR{}))
            surface_format_it;

    for (auto &request_surface_format : request_surface_formats) {
        surface_format_it = std::find_if(
                std::begin(surface_formats),
                std::end(surface_formats),
                [&request_surface_format](const VkSurfaceFormatKHR &surface) {
                    return surface.format == request_surface_format.format &&
                           surface.colorSpace == request_surface_format.colorSpace;
                });

        if (surface_format_it != surface_formats.end()) {
            break;
        }
    }

    if (surface_format_it == std::end(surface_formats)) {
        surface_format_it = std::begin(surface_formats);

        ANLog("Requested surface format not supported. Selected default value.");
    }

    return *surface_format_it;
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
        VkCompositeAlphaFlagsKHR supported_composite_alpha
) {

    if (request_composite_alpha & supported_composite_alpha) {
        return request_composite_alpha;
    }

    ANLog("Requested composite alpha not supported. Selected default value.");

    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    for (VkCompositeAlphaFlagBitsKHR composite_alpha : composite_alpha_flags) {
        if (composite_alpha & supported_composite_alpha) {
            return composite_alpha;
        }
    }

    throw std::runtime_error("No compatible composite alpha found.");
}

inline VkImageUsageFlags choose_image_usage(VkImageUsageFlags requested_image_usage, VkImageUsageFlags supported_image_usage) {
    if (requested_image_usage & supported_image_usage) {
        return requested_image_usage;
    }

    VkImageUsageFlagBits image_usage_flags[] = {
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_IMAGE_USAGE_STORAGE_BIT,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };

    for (VkImageUsageFlagBits image_usage : image_usage_flags) {
        if (image_usage & supported_image_usage) {
            return image_usage;
        }
    }

    throw std::runtime_error("No compatible image usage found.");
}

}


bool SwapChain::init(const SwapChainDescriptor &swapChainDescriptor) {
    _device = swapChainDescriptor.device;
    surface = swapChainDescriptor.surface;
    image_count = swapChainDescriptor.imageCount;
    image_usage = swapChainDescriptor.imageUsage;
    transform = swapChainDescriptor.transform;
    present_mode = swapChainDescriptor.presentMode;


    VkSurfaceCapabilitiesKHR surface_capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->vkPhysicalDevice(), surface, &surface_capabilities);

#ifdef VK_CHECK
#undef VK_CHECK
#endif
#define VK_CHECK(x)                                                                  \
	do                                                                               \
	{                                                                                \
		VkResult err = x;                                                            \
		if (err)                                                                     \
		{                                                                            \
			ANLog("Detected Vulkan error: " #x); \
			return false;                                                            \
		}                                                                            \
	} while (0)

    uint32_t surface_format_count{0U};
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_device->vkPhysicalDevice(), surface, &surface_format_count, nullptr));

    std::vector<VkSurfaceFormatKHR> surface_formats{surface_format_count};
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_device->vkPhysicalDevice(), surface, &surface_format_count, surface_formats.data()));

    uint32_t present_mode_count{0U};
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(_device->vkPhysicalDevice(), surface, &present_mode_count, nullptr));

    std::vector<VkPresentModeKHR> present_modes{present_mode_count};
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(_device->vkPhysicalDevice(), surface, &present_mode_count, present_modes.data()));

    VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

    create_info.minImageCount = detail::choose_image_count(swapChainDescriptor.imageCount, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
    create_info.imageExtent   = detail::choose_extent(swapChainDescriptor.extent,
                                                      surface_capabilities.minImageExtent,
                                                      surface_capabilities.maxImageExtent,
                                                      surface_capabilities.currentExtent);

    VkSurfaceFormatKHR request_surface_formats[] = {
            {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
    };

    auto surface_format = detail::choose_surface_format(request_surface_formats, surface_formats);

    create_info.imageFormat     = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;

    create_info.imageArrayLayers = detail::choose_image_array_layers(1U, surface_capabilities.maxImageArrayLayers);
    create_info.imageUsage       = detail::choose_image_usage(image_usage, surface_capabilities.supportedUsageFlags);

    create_info.preTransform   = detail::choose_transform(swapChainDescriptor.transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
    create_info.compositeAlpha = detail::choose_composite_alpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, surface_capabilities.supportedCompositeAlpha);
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
    this->image_usage  = create_info.imageUsage;


#undef VK_CHECK
    return true;
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
}