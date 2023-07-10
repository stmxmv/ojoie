//
// Created by Aleudillonam on 8/28/2022.
//


#include "Render/private/vulkan/Device.hpp"
#include "Core/Window.hpp"
#include "Render/private/vulkan/Instance.hpp"

#include <filesystem>
#include <fstream>

#ifdef AN_DEBUG
#define VMA_DEBUG_LOG
#endif
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace AN::VK {

inline VkImageUsageFlags choose_image_usage(VkImageUsageFlags requested_image_usage,
                                            VkImageUsageFlags supported_image_usage) {

    VkImageUsageFlags mask = requested_image_usage & supported_image_usage;

    if (mask == requested_image_usage) {
        return requested_image_usage;
    }

    throw std::runtime_error("No compatible swapchain image usage found.");
}

static constexpr VkImageUsageFlags DefaultSwapchainUsageFlag = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;

static constexpr VkSurfaceFormatKHR request_surface_formats[] = {
        {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
};


#define VK_CHECK(x)                              \
    do {                                         \
        VkResult err = x;                        \
        if (err) {                               \
            ANLog("Detected Vulkan error: " #x); \
            return false;                        \
        }                                        \
    } while (0)

bool Device::init(const DeviceDescriptor &deviceDescriptor) {
    instance        = deviceDescriptor.instance;
    _physicalDevice = deviceDescriptor.physicalDevice;

    VkPhysicalDeviceFeatures2 requestedFeatures = deviceDescriptor.requestedFeatures;

    VkPhysicalDeviceFeatures2        features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    VkPhysicalDeviceVulkan12Features deviceFeature12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features2.pNext = &deviceFeature12;

    vkGetPhysicalDeviceFeatures2(_physicalDevice, &features2);
    features = features2.features;

    /// TODO check features satisfy or not

    // Check whether ASTC is supported
    if (features.textureCompressionASTC_LDR) {
        requestedFeatures.features.textureCompressionASTC_LDR = VK_TRUE;
    }

    // Gpu properties
    vkGetPhysicalDeviceProperties(_physicalDevice, &properties);
    ANLog("GPU: %s", properties.deviceName);


    /// query device queue info

#ifdef _WIN32
    /// register window class
    WNDCLASSEXW wcex;

    wcex.cbSize     = sizeof(WNDCLASSEX);
    wcex.style      = CS_OWNDC;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;

    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);//NOLINT
    wcex.hbrBackground = nullptr;

    wcex.hIcon   = nullptr;
    wcex.hIconSm = nullptr;

    wcex.lpszClassName = L"ojoie.vulkan.test.window";
    wcex.lpszMenuName  = nullptr;

    wcex.hInstance = GetModuleHandleW(nullptr);
    wcex.lpfnWndProc = DefWindowProc;

    if (!RegisterClassExW(&wcex)) {
        AN_LOG(Error, "cannot register win32 window class");
        return false;
    }

    HWND hWnd = CreateWindowW(wcex.lpszClassName,
                              L"vulkan device test window",
                              WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              1920, 1080, nullptr, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    if (!hWnd) {
        AN_LOG(Error, "cannot create temporary win32 window");
        return false;
    }
    /// create a temporary surface
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surfaceCreateInfo.hinstance = ::GetModuleHandleW(nullptr);
    surfaceCreateInfo.hwnd      = hWnd;

    VkSurfaceKHR surface;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
    uint32_t surface_format_count{0U};
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &surface_format_count, nullptr));

    std::vector<VkSurfaceFormatKHR> surface_formats{surface_format_count};
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &surface_format_count, surface_formats.data()));

    auto surface_format_filter = request_surface_formats |
                                 std::views::filter([&surface_formats](auto &&format) {
                                     return std::ranges::any_of(surface_formats, [&format](auto &&b) {
                                         return b.format == format.format && b.colorSpace == format.colorSpace;
                                     });
                                 });

    if (surface_format_filter.empty()) {
        AN_LOG(Log, "Requested surface format not supported. Selected default value.");
        surfaceFormat = surface_formats.front();
    } else {
        surfaceFormat = surface_format_filter.front();
    }

#else
#error "not implement"
#endif

    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_family_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_family_properties_count, queue_family_properties.data());


    uint32_t queueFamilyIndex = -1;
    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index) {
        const VkQueueFamilyProperties &queue_family_property = queue_family_properties[queue_family_index];

        VkBool32 present_supported{VK_FALSE};

        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, queue_family_index, surface, &present_supported));

        /// we want a queue that is graphic and support present as well
        if ((queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_supported) {
            queueFamilyIndex = queue_family_index;
            break;
        }
    }

    if (queueFamilyIndex == -1) {
        AN_LOG(Error, "cannot find vulkan suitable device queue");
        return false;
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, surface, &_surfaceCpabilities);
    _swapchainImageUsageFlags = choose_image_usage(DefaultSwapchainUsageFlag, _surfaceCpabilities.supportedUsageFlags);

#ifdef _WIN32
    vkDestroySurfaceKHR(instance, surface, nullptr);
    DestroyWindow(hWnd);
#else
#error "not implement"
#endif

    float                   queuePriority = 1.0f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    deviceQueueCreateInfo.queueCount       = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

    // Check extensions to enable Vma Dedicated Allocation
    uint32_t device_extension_count;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &device_extension_count, nullptr));
    std::vector<VkExtensionProperties> device_extensions(device_extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &device_extension_count, device_extensions.data()));

    bool can_get_memory_requirements = std::ranges::any_of(device_extensions,
                                                           [](auto &extension) {
                                                               return std::strcmp(extension.extensionName,
                                                                                  "VK_KHR_get_memory_requirements2") == 0;
                                                           });

    bool has_dedicated_allocation = std::ranges::any_of(device_extensions,
                                                        [](auto &extension) {
                                                            return std::strcmp(extension.extensionName,
                                                                               "VK_KHR_dedicated_allocation") == 0;
                                                        });

    auto extensions = deviceDescriptor.extensions;

    if (can_get_memory_requirements && has_dedicated_allocation) {
        extensions.push_back("VK_KHR_get_memory_requirements2");
        extensions.push_back("VK_KHR_dedicated_allocation");
        ANLog("Dedicated Allocation enabled");
    }


    VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    create_info.pQueueCreateInfos       = &deviceQueueCreateInfo;
    create_info.queueCreateInfoCount    = 1U;
    create_info.pNext                   = &requestedFeatures;
    create_info.enabledExtensionCount   = (uint32_t) (extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(_physicalDevice, &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create device code %s", ResultCString(result));
        return false;
    }

    if (deviceDescriptor.onlyOneDevice) {
        volkLoadDevice(handle);
    }


    VmaVulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
    vma_vulkan_func.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
    vma_vulkan_func.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_func.vkAllocateMemory                        = vkAllocateMemory;
    vma_vulkan_func.vkFreeMemory                            = vkFreeMemory;
    vma_vulkan_func.vkMapMemory                             = vkMapMemory;
    vma_vulkan_func.vkUnmapMemory                           = vkUnmapMemory;
    vma_vulkan_func.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vma_vulkan_func.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vma_vulkan_func.vkBindBufferMemory                      = vkBindBufferMemory;
    vma_vulkan_func.vkBindImageMemory                       = vkBindImageMemory;
    vma_vulkan_func.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vma_vulkan_func.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vma_vulkan_func.vkCreateBuffer                          = vkCreateBuffer;
    vma_vulkan_func.vkDestroyBuffer                         = vkDestroyBuffer;
    vma_vulkan_func.vkCreateImage                           = vkCreateImage;
    vma_vulkan_func.vkDestroyImage                          = vkDestroyImage;
    vma_vulkan_func.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
    vma_vulkan_func.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
    vma_vulkan_func.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
    vma_vulkan_func.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vma_vulkan_func.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_func.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;


    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice   = _physicalDevice;
    allocator_info.device           = handle;
    allocator_info.instance         = deviceDescriptor.instance;

    if (can_get_memory_requirements && has_dedicated_allocation) {
        allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }

    allocator_info.pVulkanFunctions = &vma_vulkan_func;

    result = vmaCreateAllocator(&allocator_info, &memory_allocator);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create allocator %s", ResultCString(result));
        return false;
    }

    if (!_graphicQueue.init(*this, queueFamilyIndex, queue_family_properties[queueFamilyIndex], true, 0)) {
        return false;
    }

    return true;
}

void Device::deinit() {

    if (memory_allocator != VK_NULL_HANDLE) {
        VmaTotalStatistics stats;
        vmaCalculateStatistics(memory_allocator, &stats);
        ANLog("Total vulkan device memory leaked: %llu bytes", stats.total.statistics.allocationBytes);

        //        char *statsString;
        //        vmaBuildStatsString(memory_allocator, &statsString, true);
        //        ANLog("%s", statsString);
        //        vmaFreeStatsString(memory_allocator, statsString);

        vmaDestroyAllocator(memory_allocator);

        memory_allocator = VK_NULL_HANDLE;
    }

    if (handle != VK_NULL_HANDLE) {
        vkDestroyDevice(handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}


static Device gDevice;

static VkPipelineCache gPipelineCache;

void InitializeDevice() {
    VkPhysicalDeviceFeatures deviceFeatures {
            .independentBlend  = true,
            .geometryShader    = true,
            .sampleRateShading = true,
            .samplerAnisotropy = true,
    };

    VkPhysicalDeviceFeatures2 deviceFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    deviceFeatures2.features = deviceFeatures;

    VkPhysicalDeviceVulkan12Features deviceFeature12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    deviceFeature12.imagelessFramebuffer = true;
    deviceFeature12.timelineSemaphore = true;
    deviceFeatures2.pNext = &deviceFeature12;

    VK::DeviceDescriptor deviceDescriptor{};
    deviceDescriptor.instance          = GetInstance().vKInstance();
    deviceDescriptor.physicalDevice    = GetInstance().getPhysicalDevice();
    deviceDescriptor.requestedFeatures = deviceFeatures2;
    deviceDescriptor.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
//    deviceDescriptor.extensions.push_back(VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME);
    deviceDescriptor.onlyOneDevice = true;

    ANAssert(gDevice.init(deviceDescriptor));

    /// pipeline cache
    /// retrieve pipeline cache
    VkPipelineCacheCreateInfo create_info{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

    std::vector<uint8_t> cacheData;
    if (std::filesystem::exists("Data/Caches/pipelineCache")) {
        std::ifstream cacheFile("Data/Caches/pipelineCache", std::ios::binary);
        if (cacheFile.is_open()) {
            cacheFile.seekg(0, std::ios::end);
            uint64_t read_count = static_cast<uint64_t>(cacheFile.tellg());
            cacheFile.seekg(0, std::ios::beg);

            cacheData.resize(read_count);
            cacheFile.read(reinterpret_cast<char *>(cacheData.data()), (std::streamsize)read_count);
            cacheFile.close();

            create_info.initialDataSize = cacheData.size();
            create_info.pInitialData    = cacheData.data();

        } else {
            ANLog("Data/Caches/pipelineCache could not open");
        }
    }


    if (VK_SUCCESS != vkCreatePipelineCache(gDevice.vkDevice(), &create_info, nullptr, &gPipelineCache)) {
        AN_LOG(Error, "fail to create vulkan pipeline cache");
    }

    GetRenderResourceCache().setPipelineCache(gPipelineCache);
}

void DeallocDevice() {

    /// save pipeline cache
    if (gPipelineCache) {
        size_t size;
        if (VkResult result = vkGetPipelineCacheData(gDevice.vkDevice(), gPipelineCache, &size, nullptr);
            result != VK_SUCCESS) {
            ANLog("vkGetPipelineCacheData return %s", VK::ResultCString(result));
        }

        /* Get data of pipeline cache */
        std::vector<uint8_t> data(size);
        if (VkResult result = vkGetPipelineCacheData(gDevice.vkDevice(), gPipelineCache, &size, data.data());
            result != VK_SUCCESS) {
            ANLog("vkGetPipelineCacheData return %s", VK::ResultCString(result));
        }

        /* Write pipeline cache data to a file in binary format */
        if (!std::filesystem::exists("Data/Caches") || !std::filesystem::is_directory("Data/Caches")) {
            std::filesystem::create_directories("Data/Caches");
        }

        std::ofstream cacheFile("Data/Caches/pipelineCache", std::ios::binary | std::ios::trunc);
        cacheFile.write((const char *)data.data(), (std::streamsize)data.size());
        cacheFile.close();

        /* Destroy Vulkan pipeline cache */
        vkDestroyPipelineCache(gDevice.vkDevice(), gPipelineCache, nullptr);

        gPipelineCache = nullptr;
    }

    gDevice.deinit();
}

Device &GetDevice() {
    return gDevice;
}

}// namespace AN::VK