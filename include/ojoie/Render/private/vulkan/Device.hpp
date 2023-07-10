//
// Created by Aleudillonam on 8/28/2022.
//

#ifndef OJOIE_DEVICE_HPP
#define OJOIE_DEVICE_HPP

#include "ojoie/Configuration/typedef.h"
#include "ojoie/Render/private/vulkan.hpp"

#include <ojoie/Render/private/vulkan/CommandPool.hpp>
#include <ojoie/Render/private/vulkan/FencePool.hpp>
#include <ojoie/Render/private/vulkan/Queue.hpp>

#include "ojoie/Render/private/vulkan/RenderResourceCache.hpp"

namespace AN::VK {

struct DeviceDescriptor {
    VkInstance                instance;
    VkPhysicalDevice          physicalDevice;
    std::vector<const char *> extensions;
    VkPhysicalDeviceFeatures2 requestedFeatures;
    bool                      onlyOneDevice;
};

class Device : private NonCopyable {
    VkInstance instance;

    VkPhysicalDevice _physicalDevice;

    VkPhysicalDeviceFeatures features;

    uint32_t queue_family_count;

    VkDevice handle{};

    VmaAllocator memory_allocator{};

    VkPhysicalDeviceProperties properties;

    VkSurfaceFormatKHR       surfaceFormat;
    VkImageUsageFlags        _swapchainImageUsageFlags;
    VkSurfaceCapabilitiesKHR _surfaceCpabilities;
    /// the graphic device queue that support present
    Queue _graphicQueue;

public:
    Device() = default;

    ~Device() {
        deinit();
    }

    bool init(const DeviceDescriptor &deviceDescriptor);

    void deinit();

    VkDevice vkDevice() const {
        return handle;
    }

    VkPhysicalDevice vkPhysicalDevice() const {
        return _physicalDevice;
    }

    VkInstance vkInstance() const {
        return instance;
    }

    VmaAllocator vmaAllocator() const {
        return memory_allocator;
    }

    void waitIdle() const {
        vkDeviceWaitIdle(handle);
    }

    Queue &getGraphicsQueue() { return _graphicQueue; }

    const VkPhysicalDeviceProperties &getPhysicalDeviceProperties() const {
        return properties;
    }

    /// we assume VkSurfaceFormatKHR won't change during runtime
    const VkSurfaceFormatKHR &getVkSurfaceFormatKHR() const { return surfaceFormat; }

    /// we assume swapchain image usage flag won't change
    VkImageUsageFlags getSwapchainImageUsageFlags() const { return _swapchainImageUsageFlags; }

    const VkSurfaceCapabilitiesKHR &getSurfaceCpabilities() const { return _surfaceCpabilities; }
};

void    InitializeDevice();
void    DeallocDevice();
Device &GetDevice();

}// namespace AN::VK

#endif//OJOIE_DEVICE_HPP
