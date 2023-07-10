//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_VK_BUFFERPOOL_HPP
#define OJOIE_VK_BUFFERPOOL_HPP

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Buffer.hpp"

#include <algorithm>

namespace AN::VK {

class BufferAllocation : private NonCopyable {
    Buffer *buffer{nullptr};

    VkDeviceSize base_offset{0};

    VkDeviceSize _size{0};

public:
    BufferAllocation() = default;

    BufferAllocation(Buffer &buffer, VkDeviceSize size, VkDeviceSize offset)
        : buffer(&buffer), base_offset(offset), _size(size) {

    }

    BufferAllocation(BufferAllocation &&other) noexcept
        : buffer(other.buffer), base_offset(other.base_offset), _size(other._size) {
        other.buffer = nullptr;
        other.base_offset = 0;
        other._size = 0;
    }

    bool empty() const {
        return _size == 0;
    }

    void *map() {
        return (char *)buffer->map() + base_offset;
    }

    void unmap() {
        buffer->unmap();
    }

    VkDeviceSize getSize() const {
        return _size;
    }

    VkDeviceSize getOffset() const {
        return base_offset;
    }

    Buffer &getBuffer() {
        return *buffer;
    }

};


class BufferBlock : private NonCopyable {
    Buffer buffer;

    // Memory alignment, it may change according to the usage
    VkDeviceSize alignment;

    // Current offset, it increases on every allocation
    VkDeviceSize offset, _size;
public:
    BufferBlock() = default;

    BufferBlock(BufferBlock &&) = default;

    ~BufferBlock() {
        deinit();
    }

    bool init(Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocationFlag) {
        _size = size;
        offset = 0;
        BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = size;
        bufferDescriptor.memoryUsage = memory_usage;
        bufferDescriptor.bufferUsage = usage;
        bufferDescriptor.allocationFlag = allocationFlag;

        if (!buffer.init(device, bufferDescriptor)) {
            return false;
        }

        if (usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            alignment = device.getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
        } else if (usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            alignment = device.getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
        } else if (usage == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) {
            alignment = device.getPhysicalDeviceProperties().limits.minTexelBufferOffsetAlignment;
        } else if (usage == VK_BUFFER_USAGE_INDEX_BUFFER_BIT ||
                   usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
                   usage == VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT ||
                   usage == VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
            // Used to calculate the offset, required when allocating memory (its value should be power of 2)
            alignment = 16;
        } else {
            ANLog("Vulkan buffer usage not recognised");
            return false;
        }

        buffer.map();
        return true;
    }

    void deinit() {
        buffer.deinit();
    }

    /**
	 * @return An usable view on a portion of the underlying buffer
	 */
    BufferAllocation allocate(uint32_t allocate_size) {
        assert(allocate_size > 0 && "Allocation size must be greater than zero");

        auto aligned_offset = (offset + alignment - 1) & ~(alignment - 1);

        if (aligned_offset + allocate_size > _size) {
            // No more space available from the underlying buffer, return empty allocation
            return BufferAllocation{};
        }

        // Move the current offset and return an allocation
        offset = aligned_offset + allocate_size;
        return BufferAllocation{buffer, allocate_size, aligned_offset};
    }

    VkDeviceSize getSize() const {
        return _size;
    }

    void reset() {
        offset = 0;
    }

};

class BufferPool : private NonCopyable {
    Device *_device;

    /// List of blocks requested
    std::vector<BufferBlock> buffer_blocks;

    /// Minimum size of the blocks
    VkDeviceSize blockSize;

    VkBufferUsageFlags bufferUsage;

    VmaAllocationCreateFlags allocationFlag;

    VmaMemoryUsage memoryUsage;

    /// Numbers of active blocks from the start of buffer_blocks
    uint32_t active_buffer_block_count;

public:

    BufferPool() = default;

    BufferPool(BufferPool &&other) noexcept : _device(other._device), buffer_blocks(std::move(other.buffer_blocks)),
                                              blockSize(other.blockSize), bufferUsage(other.bufferUsage),
                                              allocationFlag(other.allocationFlag), memoryUsage(other.memoryUsage),
                                              active_buffer_block_count(other.active_buffer_block_count) {
        other._device = nullptr;
    }

    ~BufferPool() {
        deinit();
    }

    bool init(Device &device,
              VkDeviceSize block_size,
              VkBufferUsageFlags usage,
              VmaAllocationCreateFlags allocation_flag= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
              VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        _device = &device;
        blockSize = block_size;
        bufferUsage = usage;
        memoryUsage = memory_usage;
        allocationFlag = allocation_flag;
        active_buffer_block_count = 0;
        return true;
    }

    void deinit() {
        buffer_blocks.clear();
    }

    BufferBlock *bufferBlock(VkDeviceSize minimum_size) {
        // Find the first block in the range of the inactive blocks
        // which size is greater than the minimum size
        auto it = std::upper_bound(buffer_blocks.begin() + active_buffer_block_count, buffer_blocks.end(), minimum_size,
                                   [](const VkDeviceSize &a, const BufferBlock &b) -> bool { return a < b.getSize(); });

        if (it != buffer_blocks.end()) {
            // Recycle inactive block
            active_buffer_block_count++;
            return &*it;
        }

        uint64_t size = std::max(blockSize, minimum_size);

        ANLog("Building #%zu buffer block size %llu kb usage %#010x", buffer_blocks.size(), size >> 10, bufferUsage);

        // Create a new block, store and return it
        buffer_blocks.emplace_back();

        if (!buffer_blocks.back().init(*_device, size, bufferUsage, memoryUsage, allocationFlag)) {
            return nullptr;
        }

        auto &block = buffer_blocks[active_buffer_block_count++];

        return &block;
    }

    void reset() {
        for (BufferBlock &bufferBlock : buffer_blocks) {
            bufferBlock.reset();
        }
        active_buffer_block_count = 0;
    }

};


}

#endif//OJOIE_BUFFERPOOL_HPP
