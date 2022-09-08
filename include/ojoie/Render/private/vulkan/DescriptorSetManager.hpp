//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_DESCRIPTORSETMANAGER_HPP
#define OJOIE_DESCRIPTORSETMANAGER_HPP

#include "Render/private/vulkan/hash.hpp"
#include "Math/LRUCache.hpp"

#include <map>

namespace AN::VK {

class DescriptorAllocator : private NonCopyable {
public:
    struct PoolSizes {
        std::vector<std::pair<VkDescriptorType,float>> sizes =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.f },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1.f },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2.f },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2.f },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
                };
    };

private:

    VkDescriptorPool currentPool{VK_NULL_HANDLE};
    PoolSizes descriptorSizes;

    std::vector<std::pair<VkDescriptorSetLayout, VkDescriptorSet>> pendingDeallocatedDescriptorSets;
    std::unordered_multimap<VkDescriptorSetLayout, VkDescriptorSet> freeDescriptorSets;
    std::vector<VkDescriptorPool> usedPools;
    std::vector<VkDescriptorPool> freePools;

    VkDevice device;

    VkDescriptorPool grab_pool();

public:

    DescriptorAllocator() = default;

    DescriptorAllocator(DescriptorAllocator &&other) noexcept
        : currentPool(other.currentPool), descriptorSizes(other.descriptorSizes),
          pendingDeallocatedDescriptorSets(other.pendingDeallocatedDescriptorSets),
          freeDescriptorSets(other.freeDescriptorSets), usedPools(other.usedPools), freePools(other.freePools),
          device(other.device) {
        other.pendingDeallocatedDescriptorSets.clear();
        other.freeDescriptorSets.clear();
        other.usedPools.clear();
        other.freePools.clear();
    }

    ~DescriptorAllocator() {
        deinit();
    }

    bool init(VkDevice newDevice);

    void deinit();

    void reset_pools();

    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

    void pendingDeallocate(VkDescriptorSet set, VkDescriptorSetLayout layout);

    void deallocate(VkDescriptorSet set, VkDescriptorSetLayout layout);

    /// called by renderer
    void clearPendingDeallocatedDescriptorSets();

    VkDevice vkDevice() const {
        return device;
    }
};


struct DescriptorSetInfo {
    VkDescriptorSetLayout layout;
    std::map<uint32_t/*binding*/, VkDescriptorBufferInfo> bufferInfos;
    std::map<uint32_t/*binding*/, VkDescriptorImageInfo> imageInfos;
    std::map<uint32_t/*binding*/, VkDescriptorType> descriptorTypes;

    void clear() {
        bufferInfos.clear();
        imageInfos.clear();
        descriptorTypes.clear();
    }

    size_t hashValue() const {
        size_t value{};
        hash_param(value, layout, bufferInfos, imageInfos);
        return value;
    }

    std::vector<VkWriteDescriptorSet> generateWriteDescriptorSet(VkDescriptorSet dstSet) const {
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.reserve(descriptorTypes.size());

        for (const auto &[binding, bufferInfo] : bufferInfos) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = dstSet;
            write.dstBinding = binding;

            write.descriptorCount = 1;

            write.pBufferInfo = &bufferInfo;
            write.descriptorType = descriptorTypes.at(binding);

            writeDescriptorSets.push_back(write);
        }

        for (const auto &[binding, imageInfo] : imageInfos) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = dstSet;
            write.dstBinding = binding;

            write.descriptorCount = 1;

            write.pImageInfo = &imageInfo;
            write.descriptorType = descriptorTypes.at(binding);

            writeDescriptorSets.push_back(write);
        }

        return writeDescriptorSets;
    }

};

class DescriptorSetManager : private NonCopyable {
    constexpr static int cache_descriptor_set_capacity = 1080;
    constexpr static int pending_max_num = 108;
    bool cacheDescriptorSet = true;
    bool didClear{};
    int pendingCount{};

    typedef std::pair<VkDescriptorSet, VkDescriptorSetLayout> CacheItem;

    VkDevice _device;
    LRUCache<size_t, CacheItem> setCache{ cache_descriptor_set_capacity };
    DescriptorAllocator allocator;

    void updateDescriptorSet(const DescriptorSetInfo &info, VkDescriptorSet set) {
        std::vector<VkWriteDescriptorSet> writes = info.generateWriteDescriptorSet(set);
        vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
    }

public:

    DescriptorSetManager() = default;

    DescriptorSetManager(DescriptorSetManager &&other) noexcept
        : cacheDescriptorSet(other.cacheDescriptorSet), didClear(other.didClear), pendingCount(other.pendingCount), _device(other._device),
          setCache(std::move(other.setCache)), allocator(std::move(other.allocator)) {
        other.didClear = false;
        other.pendingCount = 0;
    }

    ~DescriptorSetManager() {
        deinit();
    }

    bool init(VkDevice device) {
        _device = device;
        allocator.init(device);
        return true;
    }


    void deinit() {
        allocator.deinit();
    }

    VkDescriptorSet getPersistentDescriptorSet(const DescriptorSetInfo &info) {
        VkDescriptorSet set;
        allocator.allocate(&set, info.layout);
        updateDescriptorSet(info, set);
        return set;
    }

    void pendingReturnPersistentDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout) {
        allocator.pendingDeallocate(set, layout);
    }

    VkDescriptorSet descriptorSet(const DescriptorSetInfo &info) {

        if (!cacheDescriptorSet) {
            VkDescriptorSet set;
            allocator.allocate(&set, info.layout);
            allocator.pendingDeallocate(set, info.layout);

            updateDescriptorSet(info, set);
            return set;
        }

        CacheItem cacheItem;
        size_t hashValue = info.hashValue();
        if (setCache.get(hashValue, cacheItem)) {
            return cacheItem.first;
        }

        cacheItem.second = info.layout;

        allocator.allocate(&cacheItem.first, info.layout);

        if (auto deleted = setCache.set(hashValue, cacheItem)) {
            allocator.pendingDeallocate(deleted->first, deleted->second);
            ++pendingCount;
        }

        updateDescriptorSet(info, cacheItem.first);

        return cacheItem.first;
    }


    void clearFrameSets() {
        if (cacheDescriptorSet) {

            if (pendingCount > pending_max_num) {
                allocator.clearPendingDeallocatedDescriptorSets();
                didClear = true;
                pendingCount = 0;
            }

        } else {

            allocator.clearPendingDeallocatedDescriptorSets();
        }

    }

    void clear() {
        if (cacheDescriptorSet) {
            setCache.clear();
        }
        allocator.reset_pools();

        didClear = true;
        pendingCount = 0;
    }


};




}

#endif//OJOIE_DESCRIPTORSETMANAGER_HPP
