//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Buffer.hpp"

#include "Render/Buffer.hpp"

namespace AN::VK {

#define VK_CHECK(statement)                                           \
    do {                                                              \
        if (VkResult result = statement; result != VK_SUCCESS) {      \
            ANLog("%s return %s", #statement, ResultCString(result)); \
        }                                                             \
    } while (0)

bool Buffer::init(Device &device, const BufferDescriptor &bufferDescriptor) {
    _device = &device;
    size = bufferDescriptor.size;
    mapped_data = nullptr;
    mapped = false;

    assert(((bufferDescriptor.allocationFlag & VMA_ALLOCATION_CREATE_MAPPED_BIT) == 0) && "Buffer memory should be mapped explicitly outside the constructor");

    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.usage = bufferDescriptor.bufferUsage;
    buffer_info.size  = size;

    VmaAllocationCreateInfo memory_info{};
    memory_info.flags = bufferDescriptor.allocationFlag;
    memory_info.usage = bufferDescriptor.memoryUsage;

    VmaAllocationInfo alloc_info{};
    VkResult result = vmaCreateBuffer(device.vmaAllocator(),
                                  &buffer_info, &memory_info,
                                  &handle, &memory,
                                  &alloc_info);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create vulkan buffer %s", ResultCString(result));
        return false;
    }

    VkMemoryPropertyFlags memoryPropertyFlags;
    vmaGetMemoryTypeProperties(device.vmaAllocator(), alloc_info.memoryType, &memoryPropertyFlags);

    isCoherent = memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    return true;
}

void Buffer::deinit() {
    if (handle != VK_NULL_HANDLE && memory != VK_NULL_HANDLE) {
        unmap();
        vmaDestroyBuffer(_device->vmaAllocator(), handle, memory);
        handle = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    }
}

void *Buffer::map() {
    if (!mapped_data) {
        VK_CHECK(vmaMapMemory(_device->vmaAllocator(), memory, &mapped_data));
        mapped = true;
    }
    return mapped_data;
}

void Buffer::unmap() {
    if (mapped) {
        vmaUnmapMemory(_device->vmaAllocator(), memory);
        mapped_data = nullptr;
        mapped      = false;
    }
}
void Buffer::flush() {
    if (!isCoherent) {
        vmaFlushAllocation(_device->vmaAllocator(), memory, 0, size);
    }
}


void Buffer::invalidate() {
    if (!isCoherent) {
        vmaInvalidateAllocation(_device->vmaAllocator(),
                                memory,
                                0,
                                size);
    }
}

}

namespace AN::RC {

VkBufferUsageFlags toVkBufferUsageFlags(BufferUsageFlag flag) {
    VkBufferUsageFlags ret{};

    if ((flag & BufferUsageFlag::TransferSource) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if ((flag & BufferUsageFlag::TransferDestination) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if ((flag & BufferUsageFlag::VertexBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if ((flag & BufferUsageFlag::IndexBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if ((flag & BufferUsageFlag::UniformBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    return ret;
}

VmaMemoryUsage toVmaMemoryUsage(MemoryUsage usage) {
    switch (usage) {
        case MemoryUsage::Auto:
            return VMA_MEMORY_USAGE_AUTO;
        case MemoryUsage::AutoPreferDevice:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case MemoryUsage::AutoPreferHost:
            return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        case MemoryUsage::Lazy:
            return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    }
    return {};
}

VmaAllocationCreateFlags toVmaAllocationCreateFlags(AllocationFlag flag) {
    VmaAllocationCreateFlags ret{};
    if ((flag & AllocationFlag::HostAccessRandom) != AllocationFlag::None) {
        ret |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }

    if ((flag & AllocationFlag::HostAccessSequentialWrite) != AllocationFlag::None) {
        ret |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    return ret;
}

Buffer::Buffer() : impl(new VK::Buffer()) {}

Buffer::~Buffer() {
    deinit();
    delete (VK::Buffer *)impl;
}

bool Buffer::init(Device &device, const BufferDescriptor &bufferDescriptor) {

    VK::Buffer *self = (VK::Buffer *)impl;

    VK::BufferDescriptor vkBufferDescriptor;

    vkBufferDescriptor.size = bufferDescriptor.size;
    vkBufferDescriptor.bufferUsage = toVkBufferUsageFlags(bufferDescriptor.bufferUsage);
    vkBufferDescriptor.memoryUsage = toVmaMemoryUsage(bufferDescriptor.memoryUsage);
    vkBufferDescriptor.allocationFlag = toVmaAllocationCreateFlags(bufferDescriptor.allocationFlag);

    return self->init(**(VK::Device **)&device, vkBufferDescriptor);
}

void Buffer::deinit() {
    if (impl) {
        VK::Buffer *self = (VK::Buffer *)impl;
        self->deinit();
    }
}

void *Buffer::map() {
    VK::Buffer *self = (VK::Buffer *)impl;
    return self->map();
}

void Buffer::unmap() {
    VK::Buffer *self = (VK::Buffer *)impl;
    self->unmap();
}

void Buffer::flush() {
    VK::Buffer *self = (VK::Buffer *)impl;
    self->flush();
}

void Buffer::invalidate() {
    VK::Buffer *self = (VK::Buffer *)impl;
    self->invalidate();
}


}