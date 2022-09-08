//
// Created by Aleudillonam on 8/28/2022.
//

#ifndef OJOIE_DEVICE_HPP
#define OJOIE_DEVICE_HPP

#include <ojoie/Core/typedef.h>
#include "Render/private/vulkan.hpp"

#include <Render/private/vulkan/FencePool.hpp>
#include <Render/private/vulkan/Queue.hpp>
#include <Render/private/vulkan/CommandPool.hpp>

#include "Render/private/vulkan/RenderResourceCache.hpp"

namespace AN::VK {

struct DeviceDescriptor {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    std::vector<const char *> extensions;
    VkPhysicalDeviceFeatures requestedFeatures;
    bool onlyOneDevice;
};

class Device : private NonCopyable {
    VkInstance instance;

    VkPhysicalDevice _physicalDevice;

    VkPhysicalDeviceFeatures features;

    uint32_t queue_family_count;

    VkDevice handle{};

    VmaAllocator memory_allocator{};

    VkPhysicalDeviceProperties properties;

    std::vector<std::vector<Queue>> queues;

    /// A command pool associated to the primary queue
    CommandPool command_pool;

    /// A fence pool associated to the primary queue
    FencePool fence_pool;

    RenderResourceCache renderResourceCache;

public:

    Device() = default;

    Device(Device &&other) noexcept : instance(other.instance),
                                      _physicalDevice(other._physicalDevice),
                                      features(other.features),
                                      queue_family_count(other.queue_family_count),
                                      handle(other.handle),
                                      memory_allocator(other.memory_allocator),
                                      properties(other.properties),
                                      queues(std::move(other.queues)),
                                      command_pool(std::move(other.command_pool)),
                                      fence_pool(std::move(other.fence_pool)),
                                      renderResourceCache(std::move(other.renderResourceCache)) {
        other.handle = VK_NULL_HANDLE;
        other.memory_allocator = VK_NULL_HANDLE;
    }

    ~Device() {
        deinit();
    }

    bool init(const DeviceDescriptor &deviceDescriptor);

    void deinit();

    const Queue &queue(VkQueueFlags required_queue_flags, uint32_t queue_index);

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

    const Queue &graphicsQueue();

    RenderResourceCache &getRenderResourceCache() {
        return renderResourceCache;
    }

    const CommandPool &getCommandPool() const {
        return command_pool;
    }
    const FencePool &getFencePool() const {
        return fence_pool;
    }

    const VkPhysicalDeviceProperties &getPhysicalDeviceProperties() const {
        return properties;
    }
};


}

#endif//OJOIE_DEVICE_HPP
