//
// Created by Aleudillonam on 9/13/2022.
//

#ifndef OJOIE_VK_BUFFERMANAGER_HPP
#define OJOIE_VK_BUFFERMANAGER_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/BufferPool.hpp"
#include <unordered_map>
#include <vector>

namespace AN::VK {

class BufferManager : private NonCopyable {

    std::unordered_map<VkBufferUsageFlags, std::pair<BufferPool, BufferBlock *>> bufferPools;
public:

    BufferManager() = default;

    BufferManager(BufferManager &&other) noexcept: bufferPools(std::move(other.bufferPools)) {

    }

    ~BufferManager() {
        deinit();
    }

    template<typename VkBufferUsageFlagsVec>
    bool init(Device &device, VkBufferUsageFlagsVec &&supportedUsages, uint64_t blockSize) {
        for (VkBufferUsageFlagBits usage : supportedUsages) {
            std::pair<BufferPool, BufferBlock *> usage_buffer_pool = std::make_pair(BufferPool{}, nullptr);

            if (!usage_buffer_pool.first.init(device, blockSize, usage)) {
                return false;
            }

            auto res_ins_it = bufferPools.emplace(usage, std::move(usage_buffer_pool));

            if (!res_ins_it.second) {
                ANLog("Failed to insert buffer pool");
                return false;
            }
        }
        return true;
    }

    void deinit() {
        bufferPools.clear();
    }

    void reset() {
        for (auto &[_, par] : bufferPools) {
            par.first.reset();
        }
    }

    BufferAllocation buffer(VkBufferUsageFlags usage, uint64_t size) {
        // Find a pool for this usage
        auto buffer_pool_it = bufferPools.find(usage);
        if (buffer_pool_it == bufferPools.end()) {
            ANLog("No buffer pool for buffer usage %d", usage);
            return BufferAllocation{};
        }

        auto &buffer_pool  = buffer_pool_it->second.first;
        BufferBlock * &buffer_block = buffer_pool_it->second.second;

        if (!buffer_block) {
            // If there is no block associated with the pool or we are creating a buffer for each allocation,
            // request a new buffer block
            buffer_block = buffer_pool.bufferBlock(size);
        }

        if (!buffer_block) {
            return BufferAllocation{};
        }

        BufferAllocation data = buffer_block->allocate(size);

        // Check if the buffer block can allocate the requested size
        if (data.empty()) {
            buffer_block = buffer_pool.bufferBlock(size);

            std::destroy_at(&data);
            std::construct_at(&data, buffer_block->allocate(size));
        }

        return data;
    }
};

}

#endif//OJOIE_BUFFERMANAGER_HPP
