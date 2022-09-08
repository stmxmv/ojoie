//
// Created by Aleudillonam on 8/28/2022.
//




#include "Render/private/vulkan/Device.hpp"

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace AN::VK {

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

bool Device::init(const DeviceDescriptor &deviceDescriptor) {
    instance = deviceDescriptor.instance;
    _physicalDevice = deviceDescriptor.physicalDevice;
    // Check whether ASTC is supported
    VkPhysicalDeviceFeatures requestedFeatures = deviceDescriptor.requestedFeatures;
    vkGetPhysicalDeviceFeatures(_physicalDevice, &features);
    if (features.textureCompressionASTC_LDR) {
        requestedFeatures.textureCompressionASTC_LDR = VK_TRUE;
    }

    // Gpu properties
    vkGetPhysicalDeviceProperties(_physicalDevice, &properties);
    ANLog("GPU: %s", properties.deviceName);


    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_family_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_family_properties_count, queue_family_properties.data());

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_properties_count, {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
    std::vector<std::vector<float>>      queue_priorities(queue_family_properties_count);

    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index) {
        const VkQueueFamilyProperties &queue_family_property = queue_family_properties[queue_family_index];

        queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 1.0f);

        VkDeviceQueueCreateInfo &queue_create_info = queue_create_infos[queue_family_index];

        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount       = queue_family_property.queueCount;
        queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
    }

    // Check extensions to enable Vma Dedicated Allocation
    uint32_t device_extension_count;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &device_extension_count, nullptr));
    std::vector<VkExtensionProperties> device_extensions(device_extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &device_extension_count, device_extensions.data()));

    bool can_get_memory_requirements = std::find_if(std::begin(device_extensions),
                                                    std::end(device_extensions),
                                                    [](auto &extension) { return std::strcmp(extension.extensionName, "VK_KHR_get_memory_requirements2") == 0; }) != std::end(device_extensions);
    bool has_dedicated_allocation    = std::find_if(std::begin(device_extensions),
                                                    std::end(device_extensions),
                                                    [](auto &extension) { return std::strcmp(extension.extensionName, "VK_KHR_dedicated_allocation") == 0; }) != std::end(device_extensions);

    auto extensions = deviceDescriptor.extensions;

    if (can_get_memory_requirements && has_dedicated_allocation) {
        extensions.push_back("VK_KHR_get_memory_requirements2");
        extensions.push_back("VK_KHR_dedicated_allocation");
        ANLog("Dedicated Allocation enabled");
    }


    VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    create_info.pQueueCreateInfos       = queue_create_infos.data();
    create_info.queueCreateInfoCount    = (uint32_t)(queue_create_infos.size());
    create_info.pEnabledFeatures        = &requestedFeatures;
    create_info.enabledExtensionCount   = (uint32_t)(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(_physicalDevice, &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create device code %s", ResultCString(result));
        return false;
    }

    if (deviceDescriptor.onlyOneDevice) {
        volkLoadDevice(handle);
    }

    queues.resize(queue_family_properties_count);

    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index) {
        const VkQueueFamilyProperties &queue_family_property = queue_family_properties[queue_family_index];

        VkBool32 present_supported{VK_FALSE};

        // Only check if surface is valid to allow for headless applications
        if (deviceDescriptor.surface != VK_NULL_HANDLE) {
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, queue_family_index, deviceDescriptor.surface, &present_supported));
        }

        for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index) {
            queues[queue_family_index].emplace_back(handle, queue_family_index, queue_family_property, present_supported, queue_index);
        }
    }

    VmaVulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
    vma_vulkan_func.vkFreeMemory = vkFreeMemory;
    vma_vulkan_func.vkMapMemory = vkMapMemory;
    vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
    vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
    vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
    vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
    vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
    vma_vulkan_func.vkCreateImage = vkCreateImage;
    vma_vulkan_func.vkDestroyImage = vkDestroyImage;
    vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
    vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vma_vulkan_func.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_func.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;



    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.physicalDevice = _physicalDevice;
    allocator_info.device         = handle;
    allocator_info.instance = deviceDescriptor.instance;

    if (can_get_memory_requirements && has_dedicated_allocation) {
        allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }

    allocator_info.pVulkanFunctions = &vma_vulkan_func;

    result = vmaCreateAllocator(&allocator_info, &memory_allocator);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create allocator %s", ResultCString(result));
    }


    return command_pool.init(handle, queue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).getFamilyIndex()) &&
           fence_pool.init(handle) && renderResourceCache.init(*this);

}

void Device::deinit() {
    command_pool.deinit();
    fence_pool.deinit();

    renderResourceCache.deinit();

    if (memory_allocator != VK_NULL_HANDLE) {
        //		VmaStats stats;
        //		vmaCalculateStats(memory_allocator, &stats);
        //
        //		LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

        vmaDestroyAllocator(memory_allocator);

        memory_allocator = VK_NULL_HANDLE;
    }

    if (handle != VK_NULL_HANDLE) {
        vkDestroyDevice(handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}


const Queue &Device::queue(VkQueueFlags required_queue_flags, uint32_t queue_index) {
    for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index) {
        Queue &first_queue = queues[queue_family_index][0];

        VkQueueFlags queue_flags = first_queue.getProperties().queueFlags;
        uint32_t     queue_count = first_queue.getProperties().queueCount;

        if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count) {
            return queues[queue_family_index][queue_index];
        }
    }

    throw std::runtime_error("Vulkan Device Queue not found");
}

const Queue &Device::graphicsQueue() {
    for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index) {
        Queue &first_queue = queues[queue_family_index][0];

        uint32_t queue_count = first_queue.getProperties().queueCount;

        if (first_queue.canPresent() && 0 < queue_count) {
            return queues[queue_family_index][0];
        }
    }

    return queue(VK_QUEUE_GRAPHICS_BIT, 0);
}

}