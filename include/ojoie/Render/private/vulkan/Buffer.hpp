//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_VK_BUFFER_HPP
#define OJOIE_VK_BUFFER_HPP

#include "Render/private/vulkan.hpp"

namespace AN::VK {

struct BufferDescriptor {
    VkDeviceSize size;
    VkBufferUsageFlags bufferUsage;
    VmaMemoryUsage memoryUsage;
    VmaAllocationCreateFlags allocationFlag;
};


class Buffer {
    Device *_device;

    VkBuffer handle{VK_NULL_HANDLE};

    VmaAllocation memory{VK_NULL_HANDLE};

    VkDeviceSize size;

    void *mapped_data;

    /// Whether it has been mapped with vmaMapMemory
    bool mapped;

    bool isCoherent;
public:

    Buffer() = default;

    Buffer(Buffer &&other) noexcept
        : _device(other._device), handle(other.handle), memory(other.memory), size(other.size), mapped_data(other.mapped_data), mapped(other.mapped) {
        other.handle = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
    }

    ~Buffer() {
        deinit();
    }

    bool init(Device &device, const BufferDescriptor &bufferDescriptor);

    void deinit();

    VkBuffer vkBuffer() const {
        return handle;
    }

    void *map();

    void unmap();

    /// \brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
    void flush();

    /// \brief invalidate memory if it is HOST_VISIBLE and not HOST_COHERENT
    void invalidate();

    VkDeviceSize getSize() const {
        return size;
    }
};

}

#endif//OJOIE_VK_BUFFER_HPP
