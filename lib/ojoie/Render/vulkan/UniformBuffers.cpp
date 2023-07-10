//
// Created by aojoie on 4/21/2023.
//

#include "Render/private/vulkan/UniformBuffers.hpp"
#include "Render/RenderTypes.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

static constexpr size_t default_uniform_buffer_size = 1024;

static size_t pad_uniform_buffer_size(size_t originalSize) {

    static size_t minUboAlignment = GetDevice().getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;

    // Calculate required alignment based on minimum device offset alignment
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    return alignedSize;
}

bool UniformBuffers::init() {
    memset(&currentBufferInfo, 0, sizeof(currentBufferInfo));
    currentBufferInfo.version = -1;
    return true;
}

void UniformBuffers::deinit() {
    oldBufferInfos.push_back(currentBufferInfo);
    for (auto it = oldBufferInfos.begin(); it != oldBufferInfos.end(); ++it) {
        if (it->buffer) {
            vmaUnmapMemory(GetDevice().vmaAllocator(), it->allocation);
            vmaDestroyBuffer(GetDevice().vmaAllocator(), it->buffer, it->allocation);
        }
    }
    oldBufferInfos.clear();
}

void UniformBuffers::allocateBuffer(UInt64 perFrameSize) {

    /// we don't need to copy the old data, cause user must not keep allocation, instead allocate it each frame
    if (currentBufferInfo.buffer) {
        oldBufferInfos.push_back(currentBufferInfo);
    }

    size_t padSizePerFrame = pad_uniform_buffer_size(perFrameSize);
    size_t totalSize       = padSizePerFrame * kMaxFrameInFlight;

    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.size  = totalSize;

    VmaAllocationCreateInfo memory_info{};
    memory_info.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    memory_info.usage         = VMA_MEMORY_USAGE_AUTO;
    memory_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;// we require ubo to be coherent

    VmaAllocationInfo alloc_info{};
    VkResult          result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                               &buffer_info, &memory_info,
                                               &currentBufferInfo.buffer, &currentBufferInfo.allocation,
                                               &alloc_info);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Cannot create vulkan uniform buffer %s", ResultCString(result));
        return;
    }

    currentBufferInfo.totalSize        = totalSize;
    currentBufferInfo.freeSizePerFrame = padSizePerFrame;
    currentBufferInfo.padSizePerFrame  = padSizePerFrame;

    result = vmaMapMemory(GetDevice().vmaAllocator(), currentBufferInfo.allocation, &currentBufferInfo.mappedData);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Cannot map vulkan uniform buffer memory %s", ResultCString(result));
    }
}

void UniformBuffers::update() {

    ++currentBufferInfo.version;

    currentBufferInfo.freeSizePerFrame = currentBufferInfo.totalSize / kMaxFrameInFlight;

    /// delete old small buffers
    for (auto it = oldBufferInfos.begin(); it != oldBufferInfos.end();) {
        if (currentBufferInfo.version - it->version > kMaxFrameInFlight + 7) {// more 7 frame for safety
            vmaUnmapMemory(GetDevice().vmaAllocator(), it->allocation);
            vmaDestroyBuffer(GetDevice().vmaAllocator(), it->buffer, it->allocation);
            it = oldBufferInfos.erase(it);

        } else {
            ++it;
        }
    }

    /// TODO compare last frame and recent frame buffer size used and try to shrink buffer

    bufferSizeUsedLastFrame = bufferSizeUsedPerFrame;
    bufferSizeUsedPerFrame  = 0;
}

UniformBufferAllocation UniformBuffers::allocate(UInt64 size) {
    UniformBufferAllocation allocation{};
    allocation.size = size;

    size_t padSize = pad_uniform_buffer_size(size);

    std::lock_guard lock(spinLock);
    if (currentBufferInfo.buffer == nullptr || currentBufferInfo.freeSizePerFrame < padSize) {

        /// reallocate big uniform buffer with double size
        size = std::max(default_uniform_buffer_size, currentBufferInfo.totalSize);

        while (size < padSize) {
            size *= 2;
        }

        allocateBuffer(size);
    }


    size_t offset = currentBufferInfo.padSizePerFrame * (currentBufferInfo.version % kMaxFrameInFlight) +
                    currentBufferInfo.padSizePerFrame - currentBufferInfo.freeSizePerFrame;

    allocation.buffer     = currentBufferInfo.buffer;
    allocation.offset     = offset;
    allocation.mappedData = (char *) currentBufferInfo.mappedData + offset;

    currentBufferInfo.freeSizePerFrame -= padSize;
    bufferSizeUsedPerFrame += padSize;

    return allocation;
}

UniformBuffers &GetUniformBuffers() {
    static UniformBuffers uniformBuffers;
    return uniformBuffers;
}

}// namespace AN::VK