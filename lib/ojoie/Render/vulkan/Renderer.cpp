//
// Created by Aleudillonam on 8/12/2022.
//

#include "Render/Renderer.hpp"
#include "Core/Window.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan_descriptor_set_manager.hpp"

#include "Render/Texture.hpp"
#include "Render/UniformBuffer.hpp"
#include "Render/Sampler.hpp"

#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#ifdef OJOIE_USE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <Windows.h>

#include <unordered_map>
#include <set>

namespace AN {


static const char * validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
};

static const char * deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef AN_DEBUG
#define ENABLE_VALIDATION_LAYERS
#endif


static std::vector<const char*> getRequiredExtensions() {

    std::vector<const char*> extensions;

#ifdef OJOIE_USE_GLFW

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions.insert(extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
#endif

#ifdef ENABLE_VALIDATION_LAYERS
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif


    return extensions;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

    ANLog("validation layer: %s", pCallbackData->pMessage);

    return VK_FALSE;
}

static bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static std::vector<VkExtensionProperties> getAllAvailableExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    return extensions;
}

inline static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo                 = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static VkInstance CreateVkInstance() {

    /// create vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "ANRenderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;


#ifdef ENABLE_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (checkValidationLayerSupport()) {
        createInfo.enabledLayerCount   = std::size(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;

    } else {
        createInfo.enabledLayerCount = 0;

        ANLog("Request enable vulkan validation layers, but is not available");
    }

#else
    createInfo.enabledLayerCount = 0;
#endif
    auto extensions                    = getRequiredExtensions();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS) {
        ANLog("Failed to create Vulkan instance!\nMaybe you don't have an appropriate GPU!\n[error code]: %d", result);
        return nullptr;
    }


    return instance;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device) {
    QueueFamilyIndices indices{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }
    return indices;
}

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }



    return details;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions, deviceExtensions + std::size(deviceExtensions));

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }


    return requiredExtensions.empty();
}

static bool isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice device, QueueFamilyIndices *indices) {


    *indices                 = findQueueFamilies(surface, device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices->isComplete() && extensionsSupported && swapChainAdequate;
}


void CreateVulkanDevice(VkInstance instance, VkPhysicalDevice *physicalDevice, VkDevice *logicalDevice,
                        VkPhysicalDeviceProperties *properties, VkSurfaceKHR test_surface) {

    auto fail = [physicalDevice, logicalDevice] {
        *physicalDevice = nullptr;
        *logicalDevice = nullptr;
    };


    /// pickPhysicalDevice
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        ANLog("failed to find GPUs with Vulkan support!");
        fail();
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


    QueueFamilyIndices indices;
    for (const auto &aDevice : devices) {
        *physicalDevice = aDevice;
        if (isDeviceSuitable(test_surface, aDevice, &indices)) {
            break;
        }
        *physicalDevice = nullptr;
    }

    if (*physicalDevice == VK_NULL_HANDLE) {
        ANLog("failed to find a suitable GPU!");
        fail();
        return;
    }



    /// createLogicalDevice
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    /// device features here
    VkPhysicalDeviceFeatures deviceFeatures{
            .geometryShader = true,
            .sampleRateShading = true,
            .samplerAnisotropy = true
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos    = queueCreateInfos.data();

    createInfo.enabledExtensionCount   = std::size(deviceExtensions);
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    createInfo.pEnabledFeatures = &deviceFeatures;


#ifdef ENABLE_VALIDATION_LAYERS
        createInfo.enabledLayerCount   = std::size(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
#else
        createInfo.enabledLayerCount = 0;
#endif

    if (vkCreateDevice(*physicalDevice, &createInfo, nullptr, logicalDevice) != VK_SUCCESS) {
        ANLog("failed to create logical device!");
        fail();
        return;
    }


    vkGetPhysicalDeviceProperties(*physicalDevice, properties);

    ANLog("Vulkan gpu name: %s", properties->deviceName);
    ANLog("device max memory allocation count %d", properties->limits.maxMemoryAllocationCount);

}


VmaAllocator CreateVmaAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {

    VmaVulkanFunctions vulkanFunctions    = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = physicalDevice;
    allocatorCreateInfo.device                 = logicalDevice;
    allocatorCreateInfo.instance               = instance;
    allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

    VmaAllocator allocator;
    vmaCreateAllocator(&allocatorCreateInfo, &allocator);

    return allocator;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
//        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//            return availableFormat;
//        }
    }
    return availableFormats[0];
}


static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}


static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, Size &resolutionInOut) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = { (uint32_t)(resolutionInOut.width), (uint32_t)(resolutionInOut.height)};

    actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    resolutionInOut.width = actualExtent.width;
    resolutionInOut.height = actualExtent.height;

    return actualExtent;
}

static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw Exception("failed to find supported format!");
}

static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(physicalDevice,
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

static bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static void createRenderImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples,
                             VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                             VmaAllocator allocator, VkImage& image, VmaAllocation &allocation) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT ;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        ANLog("failed to allocate image memory!");
    }
}

static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        ANLog("failed to create texture image view!");
        return nullptr;
    }

    return imageView;
}

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

struct VulkanLayer {
    Window *window;
    VkSurfaceKHR surface;

    VkSurfaceTransformFlagBitsKHR currentTransform;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t maxImageCount;

    VkSwapchainKHR swapChain;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkImage colorImage;
    VmaAllocation colorImageAllocation;
    VkImageView colorImageView;

    VkImage depthImage;
    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;
};



struct Renderer::Impl {

    VkPhysicalDeviceProperties gpuProperties;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocator vmaAllocator;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;


    uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkRenderPass renderPass;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;


    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;

    std::unordered_map<Window *, VulkanLayer> layers;

    RC::DescriptorSetManager descriptorSetManager;
    RC::DescriptorSetInfo descriptorSetInfo;
    std::map<uint32_t, uint32_t> uniformBuffersOffsets;
    std::vector<uint32_t> descriptorSetDynamicOffsets;

#ifdef AN_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    bool createVkInstance() {
        vkInstance = CreateVkInstance();
        return vkInstance != nullptr;
    }


    bool createDevice(VkSurfaceKHR surface) {
        CreateVulkanDevice(vkInstance, &physicalDevice, &logicalDevice, &gpuProperties, surface);
        msaaSamples = getMaxUsableSampleCount(gpuProperties);
        return physicalDevice && logicalDevice;
    }

    void setupDebugMessenger() {
#ifndef ENABLE_VALIDATION_LAYERS
        return;
#endif
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT
                = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
        if (CreateDebugUtilsMessengerEXT != nullptr) {
            if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                ANLog("failed to set up debug messenger!");
            }
        }
    }

    void updateLayerSwapChainInfo(VulkanLayer &layerOut, const SwapChainSupportDetails &swapChainSupport) {
        layerOut.surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        layerOut.presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        layerOut.maxImageCount = swapChainSupport.capabilities.maxImageCount;
        layerOut.currentTransform = swapChainSupport.capabilities.currentTransform;
    }

    VkExtent2D getLayerSwapChainExtent(VulkanLayer &layer, Size &resolutionInOut) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, layer.surface);
        updateLayerSwapChainInfo(layer, swapChainSupport);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, resolutionInOut);
        return extent;
    }

    bool createLayer(Window *window, VulkanLayer &layerOut, Size &resolutionInOut) {

        if (!createLayerSurface(window, layerOut)) {
            return false;
        }

        if (physicalDevice == nullptr) [[unlikely]] {
            createDevice(layerOut.surface);
            vmaAllocator = CreateVmaAllocator(vkInstance, physicalDevice, logicalDevice);
        }


        VkExtent2D extend = getLayerSwapChainExtent(layerOut, resolutionInOut);

        return createLayerSwapChain(layerOut, extend);
    }

    bool createLayerSurface(Window *window, VulkanLayer &layerOut) {
        ANAssert(layerOut.surface == nullptr, "Vulkan layer already has a surface");
#ifdef OJOIE_USE_GLFW

        if (glfwCreateWindowSurface(vkInstance, (GLFWwindow *) window->getUnderlyingWindow(), nullptr, &layerOut.surface) != VK_SUCCESS) {
            ANLog("failed to create window surface!");
            return false;
        }

#endif
        layerOut.window = window;

        return true;
    }

    bool createLayerSwapChain(VulkanLayer &layerOut, const VkExtent2D &extent) {

        if (extent.width == 0 || extent.height == 0) {
            ANLog("Invalid swapChain extent2D");
            return false;
        }

        uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
        if (layerOut.maxImageCount > 0 && imageCount > layerOut.maxImageCount) {
            imageCount = layerOut.maxImageCount;
            ANLog("Warning: Vulkan imageCount adjusted to %u", imageCount);
        }


        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = layerOut.surface;

        swapchainCreateInfo.minImageCount    = imageCount;
        swapchainCreateInfo.imageFormat      = layerOut.surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace  = layerOut.surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent      = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices    = findQueueFamilies(layerOut.surface, physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
            ANLog("INFO: Vulkan Graphic Family is different from Present Family");
        } else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swapchainCreateInfo.preTransform   = layerOut.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode    = layerOut.presentMode;
        swapchainCreateInfo.clipped        = VK_TRUE;

        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


        if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &layerOut.swapChain) != VK_SUCCESS) {
            ANLog("failed to create swap chain!");
            return false;
        }


        vkGetSwapchainImagesKHR(logicalDevice, layerOut.swapChain, &imageCount, nullptr);
        layerOut.swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, layerOut.swapChain, &imageCount, layerOut.swapChainImages.data());

        layerOut.swapChainExtent      = extent;

        /// create image view
        layerOut.swapChainImageViews.resize(layerOut.swapChainImages.size());
        for (size_t i = 0; i < layerOut.swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                           = layerOut.swapChainImages[i];
            createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                          = layerOut.surfaceFormat.format;
            createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &layerOut.swapChainImageViews[i]) != VK_SUCCESS) {
                ANLog("failed to create image views!");
                return false;
            }
        }

        /// create color image
        createRenderImage(layerOut.swapChainExtent.width, layerOut.swapChainExtent.height, msaaSamples, layerOut.surfaceFormat.format,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    vmaAllocator, layerOut.colorImage, layerOut.colorImageAllocation);
        layerOut.colorImageView = createImageView(logicalDevice, layerOut.colorImage, layerOut.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

        /// create depth image
        VkFormat depthFormat = findDepthFormat(physicalDevice);
        createRenderImage(layerOut.swapChainExtent.width, layerOut.swapChainExtent.height, msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         vmaAllocator, layerOut.depthImage, layerOut.depthImageAllocation);

        layerOut.depthImageView = createImageView(logicalDevice, layerOut.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        /// create renderPass if needed
        if (renderPass == nullptr) {
            createRenderPass(layerOut.surfaceFormat.format);
        }

        /// create frame buffer
        layerOut.swapChainFramebuffers.resize(layerOut.swapChainImageViews.size());
        for (size_t i = 0; i < layerOut.swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                    layerOut.colorImageView, layerOut.depthImageView, layerOut.swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = std::size(attachments);
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = layerOut.swapChainExtent.width;
            framebufferInfo.height = layerOut.swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &layerOut.swapChainFramebuffers[i]) != VK_SUCCESS) {
                ANLog("failed to create framebuffer!");
                return false;
            }
        }

        return true;
    }

    void cleanupLayerSwapChain(VulkanLayer &layer) {
        for (auto &swapChainImageView : layer.swapChainImageViews) {
            if (swapChainImageView) {
                vkDestroyImageView(logicalDevice, swapChainImageView, nullptr);
                swapChainImageView = nullptr;
            }
        }
        if (layer.swapChain) {
            vkDestroySwapchainKHR(logicalDevice, layer.swapChain, nullptr);
            layer.swapChain = nullptr;
        }

        if (layer.colorImage) {
            vmaDestroyImage(vmaAllocator, layer.colorImage, layer.colorImageAllocation);
            vkDestroyImageView(logicalDevice, layer.colorImageView, nullptr);
            layer.colorImage = nullptr;
            layer.colorImageView = nullptr;
        }

        if (layer.depthImage) {
            vmaDestroyImage(vmaAllocator, layer.depthImage, layer.depthImageAllocation);
            vkDestroyImageView(logicalDevice, layer.depthImageView, nullptr);
            layer.depthImage = nullptr;
            layer.depthImageView = nullptr;
        }


        for (auto & swapChainFramebuffer : layer.swapChainFramebuffers) {
            if (swapChainFramebuffer) {
                vkDestroyFramebuffer(logicalDevice, swapChainFramebuffer, nullptr);
                swapChainFramebuffer = nullptr;
            }
        }
    }

    bool recreateLayerSwapChain(VulkanLayer &layer, Size &resolutionInOut) {
        vkDeviceWaitIdle(logicalDevice);
//        vkWaitForFences(logicalDevice, inFlightFences.size(), inFlightFences.data(), VK_TRUE, UINT64_MAX);
        cleanupLayerSwapChain(layer);
        VkExtent2D extent;
        do {
            cpu_relax();
            cpu_relax();
            extent = getLayerSwapChainExtent(layer, resolutionInOut);
        } while (extent.width == 0 || extent.height == 0);

        return createLayerSwapChain(layer, extent);
    }

    void deinitLayers() {
        vkDeviceWaitIdle(logicalDevice);

        for (auto &&[_, layer] : layers) {
            cleanupLayerSwapChain(layer);
            vkDestroySurfaceKHR(vkInstance, layer.surface, nullptr);
        }
    }

    bool createRenderPass(VkFormat format) {
        /// create render pass
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = format;
        colorAttachment.samples = msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment, colorAttachmentResolve };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = std::size(attachments);
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            ANLog("failed to create render pass!");
            return false;
        }

        return true;
    }

    template<typename Func>
    void recordCommandBuffer(VkCommandBuffer aCommandBuffer, uint32_t imageIndex, const VulkanLayer &layer, Func &&drawCalls) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(aCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = layer.swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = layer.swapChainExtent;

        VkClearValue clearValue[2]{};
        clearValue[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValue[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = std::size(clearValue);
        renderPassInfo.pClearValues = clearValue;

        vkCmdBeginRenderPass(aCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        drawCalls();

        vkCmdEndRenderPass(aCommandBuffer);

        if (vkEndCommandBuffer(aCommandBuffer) != VK_SUCCESS) {
            ANLog("failed to record command buffer!");
        }
    }
    template<typename Func>
    void drawFrame(RenderContext &context, VulkanLayer &layer, Func &&drawCalls) {

        if (fabs(context.frameWidth - 0.f) < 0.01f || fabs(context.frameHeight - 0.f) < 0.01f) {
            return;
        }


        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice, layer.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR ||
            fabs((float)layer.swapChainExtent.width - context.frameWidth) > 0.01f ||
            fabs((float)layer.swapChainExtent.height - context.frameHeight) > 0.01f) {

            Size resolution{ context.frameWidth, context.frameHeight };
            recreateLayerSwapChain(layer, resolution);

            return;
        }

        if (result != VK_SUCCESS) {
            ANLog("failed to acquire swap chain image!");
            return;
        }


        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame],  0);

        context.graphicContext->commandBuffer = commandBuffers[currentFrame];
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex, layer, std::forward<Func>(drawCalls));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            ANLog("failed to submit draw command buffer!");
            return;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { layer.swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};

Renderer::Renderer() : impl(new Impl{}){
    renderContext.graphicContext = new GraphicContext{};
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


    if (!impl->createVkInstance()) {
        return false;
    }


#ifdef AN_DEBUG
    impl->setupDebugMessenger();
#endif

    Window *window = currentWindow.load(std::memory_order_acquire);

    if (window == nullptr) {
        ANLog("No current window context");
        return false;
    }

    Size resolution{ renderContext.frameWidth, renderContext.frameHeight };

    VulkanLayer &layer = impl->layers[window];

    /// create layer also create devices renderPass, and VmaAllocator
    if (!impl->createLayer(window, layer, resolution)) {
        return false;
    }


    QueueFamilyIndices indices = findQueueFamilies(layer.surface, impl->physicalDevice);

    vkGetDeviceQueue(impl->logicalDevice, indices.graphicsFamily.value(), 0, &impl->graphicsQueue);
    vkGetDeviceQueue(impl->logicalDevice, indices.presentFamily.value(), 0, &impl->presentQueue);

    impl->graphicsQueueFamily = indices.presentFamily.value();


    /// create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    if (vkCreateCommandPool(impl->logicalDevice, &poolInfo, nullptr, &impl->commandPool) != VK_SUCCESS) {
        ANLog("failed to create command pool!");
        return false;
    }


    /// create command buffer
    impl->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = impl->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(impl->logicalDevice, &allocInfo, impl->commandBuffers.data()) != VK_SUCCESS) {
        ANLog("failed to allocate command buffers!");
        return false;
    }

    /// create sync object
    impl->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    impl->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    impl->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(impl->logicalDevice, &semaphoreInfo, nullptr, &impl->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(impl->logicalDevice, &semaphoreInfo, nullptr, &impl->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(impl->logicalDevice, &fenceInfo, nullptr, &impl->inFlightFences[i]) != VK_SUCCESS) {

            ANLog("failed to create synchronization objects for a frame!");
            return false;
        }
    }





    renderContext.maxFrameInFlight = MAX_FRAMES_IN_FLIGHT;
    renderContext.graphicContext->gpuProperties = &impl->gpuProperties;
    renderContext.graphicContext->vkInstance = impl->vkInstance;
    renderContext.graphicContext->physicalDevice = impl->physicalDevice;
    renderContext.graphicContext->logicalDevice = impl->logicalDevice;
    renderContext.graphicContext->vmaAllocator = impl->vmaAllocator;
    renderContext.graphicContext->renderPass = impl->renderPass;

    renderContext.graphicContext->commandPool = impl->commandPool;
    renderContext.graphicContext->graphicsQueueFamily = impl->graphicsQueueFamily;
    renderContext.graphicContext->graphicQueue = impl->graphicsQueue;

    renderContext.msaaSamples = impl->msaaSamples;

    renderContext.graphicContext->descriptorLayoutCache.init(impl->logicalDevice);

    if (!impl->descriptorSetManager.init(impl->logicalDevice, impl->graphicsQueue, renderContext.maxFrameInFlight)) {
        return false;
    }

    return true;
}

inline static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                 VkDebugUtilsMessengerEXT debugMessenger,
                                                 const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Renderer::willDeinit() {
    /// called before resources deinit
    resourceFence();
    isStop = true;
}

void Renderer::resourceFence() {
    if (!isStop) {
        vkQueueWaitIdle(impl->graphicsQueue);
    }
}

void Renderer::deinit() {
    vkDeviceWaitIdle(impl->logicalDevice);
    vkDestroyRenderPass(impl->logicalDevice, impl->renderPass, nullptr);
    impl->deinitLayers();

    renderContext.graphicContext->descriptorLayoutCache.deinit();
    impl->descriptorSetManager.deinit();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(impl->logicalDevice, impl->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(impl->logicalDevice, impl->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(impl->logicalDevice, impl->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(impl->logicalDevice, impl->commandPool, nullptr);

#if defined(AN_DEBUG) && defined(ENABLE_VALIDATION_LAYERS)

    DestroyDebugUtilsMessengerEXT(impl->vkInstance, impl->debugMessenger, nullptr);

#endif


    vmaDestroyAllocator(impl->vmaAllocator);
    vkDestroyDevice(impl->logicalDevice, nullptr);
    vkDestroyInstance(impl->vkInstance, nullptr);
}

void Renderer::changeNodes(const std::vector<std::shared_ptr<Node>> &nodes) {
    nodesToRender = nodes;
}

void Renderer::render(float deltaTime, float elapsedTime) {
    static Window *lastWindow = nullptr;
    if (lastWindow != currentWindow.load(std::memory_order_relaxed)) [[unlikely]] {
        lastWindow = currentWindow.load(std::memory_order_relaxed);

        auto layerIter = impl->layers.find(lastWindow);
        if (!impl->layers.contains(lastWindow)) [[unlikely]] {
            VulkanLayer &layer = impl->layers[lastWindow];
            Size resolution{ renderContext.frameWidth, renderContext.frameHeight };
            impl->createLayer(lastWindow, layer, resolution);
        }
    }


    renderContext.deltaTime = deltaTime;
    renderContext.elapsedTime = elapsedTime;

    renderContext.window = currentWindow;
    renderContext.cursorState = currentCursorState;


    VulkanLayer &layer = impl->layers[lastWindow];
    impl->drawFrame(renderContext, layer, [this] {

        impl->descriptorSetManager.clearFrameSets();

        for (auto &node : nodesToRender) {
            if (node->r_needsRender) {
                node->render(renderContext);
            }
        }

    });

    ++renderContext.frameCount;

    if (completionHandler) [[likely]] {
        completionHandler();
    }

}

void Renderer::didChangeRenderPipeline(class RC::RenderPipeline &pipeline) {
    impl->descriptorSetInfo.clear();
    impl->uniformBuffersOffsets.clear();
    VkDescriptorSetLayout descriptorSetLayout = (VkDescriptorSetLayout) pipeline.getVkDescriptorLayout();
    impl->descriptorSetInfo.layout = descriptorSetLayout;
}

void Renderer::bindUniformBuffer(uint32_t binding, RC::UniformBuffer &uniformBuffer) {
    VkDescriptorBufferInfo &bufferInfo = impl->descriptorSetInfo.bufferInfos[binding];

    bufferInfo.offset = 0;
    bufferInfo.buffer = (VkBuffer)uniformBuffer.getUnderlyingBuffer();
    bufferInfo.range = uniformBuffer.getSize();

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    impl->uniformBuffersOffsets[binding] = uniformBuffer.getOffset();
}

void Renderer::bindTexture(uint32_t binding, RC::Texture &texture) {
    VkDescriptorImageInfo &imageInfo = impl->descriptorSetInfo.imageInfos[binding];
    imageInfo.imageView = (VkImageView)texture.getUnderlyingTexture();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler = nullptr;

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

}

void Renderer::bindSampler(uint32_t binding, RC::Sampler &sampler) {
    VkDescriptorImageInfo &imageInfo = impl->descriptorSetInfo.imageInfos[binding];
    imageInfo.sampler = (VkSampler)sampler.getUnderlyingSampler();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.imageView = nullptr;

    impl->descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLER;

}

void Renderer::drawIndexed(uint32_t indexCount) {
    impl->descriptorSetDynamicOffsets.clear();
    impl->descriptorSetDynamicOffsets.reserve(impl->uniformBuffersOffsets.size());

    for (auto &[_, offset] : impl->uniformBuffersOffsets) {
        impl->descriptorSetDynamicOffsets.push_back(offset);
    }


    VkDescriptorSet descriptorSet = impl->descriptorSetManager.getDescriptorSet(impl->descriptorSetInfo);

    VkPipelineLayout pipelineLayout = (VkPipelineLayout) RC::RenderPipeline::Current()->getVkPipelineLayout();

    vkCmdBindDescriptorSets(renderContext.graphicContext->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &descriptorSet,
                            impl->descriptorSetDynamicOffsets.size(), impl->descriptorSetDynamicOffsets.data());


    vkCmdDrawIndexed(renderContext.graphicContext->commandBuffer, indexCount, 1, 0, 0, 0);
}

void Renderer::draw(uint32_t count) {

    impl->descriptorSetDynamicOffsets.clear();
    impl->descriptorSetDynamicOffsets.reserve(impl->uniformBuffersOffsets.size());

    for (auto &[_, offset] : impl->uniformBuffersOffsets) {
        impl->descriptorSetDynamicOffsets.push_back(offset);
    }


    VkDescriptorSet descriptorSet = impl->descriptorSetManager.getDescriptorSet(impl->descriptorSetInfo);

    VkPipelineLayout pipelineLayout = (VkPipelineLayout) RC::RenderPipeline::Current()->getVkPipelineLayout();

    vkCmdBindDescriptorSets(renderContext.graphicContext->commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &descriptorSet,
                            impl->descriptorSetDynamicOffsets.size(), impl->descriptorSetDynamicOffsets.data());

    vkCmdDraw(renderContext.graphicContext->commandBuffer, count, 1, 0, 0);
}

}