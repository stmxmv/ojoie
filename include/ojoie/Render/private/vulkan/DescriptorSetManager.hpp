//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_DESCRIPTORSETMANAGER_HPP
#define OJOIE_DESCRIPTORSETMANAGER_HPP

#include "ojoie/Render/private/vulkan/hash.hpp"
#include <ojoie/Utility/Log.h>
#include <ojoie/Render/RenderTypes.hpp>
#include "ojoie/Math/LRUCache.hpp"
#include <ojoie/Threads/SpinLock.hpp>
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
        layout = nullptr;
    }

    size_t hashValue() const {
        size_t value{};
        hash_param(value, layout, bufferInfos, imageInfos);
        return value;
    }

    std::vector<VkWriteDescriptorSet> generateWriteDescriptorSet(VkDescriptorSet dstSet) const;

};

class DescriptorSetManager : private NonCopyable {
    constexpr static int cache_descriptor_set_threhold = 1080;

    struct CacheItem {
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
        UInt32 version;
    };

    VkDevice _device;
    LRUCache<size_t, CacheItem> setCache;
    DescriptorAllocator allocator;

    UInt32 _version;

    SpinLock spinLock;

    void updateDescriptorSet(const DescriptorSetInfo &info, VkDescriptorSet set);

public:

    DescriptorSetManager() = default;

    ~DescriptorSetManager() {
        deinit();
    }

    bool init(VkDevice device);

    void deinit();

    /// can be called in any thread parallelly
    VkDescriptorSet getPersistentDescriptorSet(const DescriptorSetInfo &info);

    /// can be called in any thread parallelly
    void pendingReturnPersistentDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout);

    /// can be called in any thread parallelly
    VkDescriptorSet descriptorSet(const DescriptorSetInfo &info);

    /// update should happen after acquiring the next swapchain image and before requesting any descriptor set
    /// not thread safe, should sync externally
    void update();

    /// not thread safe, should sync externally
    void clear();
};


DescriptorSetManager &GetDescriptorSetManager();

}

#endif//OJOIE_DESCRIPTORSETMANAGER_HPP
