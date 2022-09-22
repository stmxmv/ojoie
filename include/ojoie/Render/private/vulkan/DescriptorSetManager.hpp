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
    DescriptorSetLayout *layout;
    std::map<uint32_t/*binding*/, std::map<uint32_t/*arrayElement*/, VkDescriptorBufferInfo>> bufferInfos;
    std::map<uint32_t/*binding*/, std::map<uint32_t/*arrayElement*/, VkDescriptorImageInfo>> imageInfos;

    void reset() {
        bufferInfos.clear();
        imageInfos.clear();
    }

    size_t hashValue() const {
        size_t value{};
        hash_param(value, layout, bufferInfos, imageInfos);
        return value;
    }

    std::vector<VkWriteDescriptorSet> generateWriteDescriptorSet(VkDescriptorSet dstSet) const {
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.reserve(bufferInfos.size() + imageInfos.size());

        // Iterate over all buffer bindings
        for (const auto &binding_it : bufferInfos) {
            auto binding          = binding_it.first;
            const auto &buffer_bindings = binding_it.second;

            if (auto binding_info = layout->getLayoutBinding(binding)) {
                // Iterate over all binding buffers in array
                for (const auto &element_it : buffer_bindings) {
                    auto arrayElement = element_it.first;
                    const auto &buffer_info = element_it.second;

                    VkWriteDescriptorSet write_descriptor_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

                    write_descriptor_set.dstBinding      = binding;
                    write_descriptor_set.descriptorType  = binding_info->descriptorType;
                    write_descriptor_set.pBufferInfo     = &buffer_info;
                    write_descriptor_set.dstSet          = dstSet;
                    write_descriptor_set.dstArrayElement = arrayElement;
                    write_descriptor_set.descriptorCount = 1;

                    writeDescriptorSets.push_back(write_descriptor_set);
                }
            } else {
                ANLog("Shader layout set does not use buffer binding at #%d", binding);
            }
        }

        // Iterate over all image bindings
        for (const auto &binding_it : imageInfos) {
            auto binding_index      = binding_it.first;
            const auto &binding_resources = binding_it.second;

            if (auto binding_info = layout->getLayoutBinding(binding_index)) {
                // Iterate over all binding images in array
                for (const auto &element_it : binding_resources) {
                    auto arrayElement = element_it.first;
                    const auto &image_info  = element_it.second;

                    VkWriteDescriptorSet write_descriptor_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

                    write_descriptor_set.dstBinding      = binding_index;
                    write_descriptor_set.descriptorType  = binding_info->descriptorType;
                    write_descriptor_set.pImageInfo      = &image_info;
                    write_descriptor_set.dstSet          = dstSet;
                    write_descriptor_set.dstArrayElement = arrayElement;
                    write_descriptor_set.descriptorCount = 1;

                    writeDescriptorSets.push_back(write_descriptor_set);
                }
            } else {
                ANLog("Shader layout set does not use image binding at #%d", binding_index);
            }
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
        allocator.allocate(&set, info.layout->vkDescriptorSetLayout());
        updateDescriptorSet(info, set);
        return set;
    }

    void pendingReturnPersistentDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout) {
        allocator.pendingDeallocate(set, layout);
    }

    VkDescriptorSet descriptorSet(const DescriptorSetInfo &info) {

        if (!cacheDescriptorSet) {
            VkDescriptorSet set;
            allocator.allocate(&set, info.layout->vkDescriptorSetLayout());
            allocator.pendingDeallocate(set, info.layout->vkDescriptorSetLayout());

            updateDescriptorSet(info, set);
            return set;
        }

        CacheItem cacheItem;
        size_t hashValue = info.hashValue();
        if (setCache.get(hashValue, cacheItem)) {
            return cacheItem.first;
        }

        cacheItem.second = info.layout->vkDescriptorSetLayout();

        allocator.allocate(&cacheItem.first, info.layout->vkDescriptorSetLayout());

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
