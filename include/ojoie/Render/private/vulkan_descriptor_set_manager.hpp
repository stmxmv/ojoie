//
// Created by Aleudillonam on 8/16/2022.
//

#ifndef OJOIE_VULKAN_DESCRIPTOR_MANAGER_HPP
#define OJOIE_VULKAN_DESCRIPTOR_MANAGER_HPP

#include "vulkan.hpp"
#include <ojoie/Core/typedef.h>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Math/LRUCache.hpp>

#include <unordered_map>

#include <vulkan/vulkan.h>



template <>
struct std::hash<VkDescriptorBufferInfo> {
    size_t operator()(const VkDescriptorBufferInfo &descriptor_buffer_info) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, descriptor_buffer_info.buffer);
        AN::Math::hash_combine(result, descriptor_buffer_info.range);
        AN::Math::hash_combine(result, descriptor_buffer_info.offset);

        return result;
    }
};

template <>
struct std::hash<VkDescriptorImageInfo> {
    size_t operator()(const VkDescriptorImageInfo &descriptor_image_info) const {
        std::size_t result = 0;

        AN::Math::hash_combine(result, descriptor_image_info.imageView);
        AN::Math::hash_combine(result, static_cast<std::underlying_type<VkImageLayout>::type>(descriptor_image_info.imageLayout));
        AN::Math::hash_combine(result, descriptor_image_info.sampler);

        return result;
    }
};

namespace AN {

template <typename T>
inline void hash_param(size_t &seed, const T &value){
    AN::Math::hash_combine(seed, value);
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &... args) {
    hash_param(seed, first_arg);
    hash_param(seed, args...);
}

template <>
inline void hash_param<std::map<uint32_t, VkDescriptorBufferInfo>>
        (size_t &seed, const std::map<uint32_t, VkDescriptorBufferInfo> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        AN::Math::hash_combine(seed, binding_set.second);
    }
}

template <>
inline void hash_param<std::map<uint32_t, VkDescriptorImageInfo>>
        (size_t &seed, const std::map<uint32_t, VkDescriptorImageInfo> &value) {
    for (const auto &binding_set : value) {
        AN::Math::hash_combine(seed, binding_set.first);
        AN::Math::hash_combine(seed, binding_set.second);
    }
}

}





namespace AN::RC {



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
    uint32_t pendingFrameCount{};
    uint32_t _maxFrameInFlight;

    typedef std::pair<VkDescriptorSet, VkDescriptorSetLayout> CacheItem;

    VkDevice _device;
    VkQueue  _graphicQueue;
    LRUCache<size_t, CacheItem> setCache{ cache_descriptor_set_capacity };
    DescriptorAllocator allocator;

    void updateDescriptorSet(const DescriptorSetInfo &info, VkDescriptorSet set) {
        std::vector<VkWriteDescriptorSet> writes = info.generateWriteDescriptorSet(set);

        /// if no cache descriptor set, allocator will allocate farthest frame used set
        if (cacheDescriptorSet && didClear) {
            if (pendingFrameCount < _maxFrameInFlight) {
                vkQueueWaitIdle(_graphicQueue);
            }
            didClear = false;
        }
        vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
    }

public:

    bool init(VkDevice device, VkQueue graphicQueue, uint32_t maxFrameInFlight) {
        _device = device;
        _graphicQueue = graphicQueue;
        _maxFrameInFlight = maxFrameInFlight;
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

    VkDescriptorSet getDescriptorSet(const DescriptorSetInfo &info) {

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

            if (didClear) {
                ++pendingFrameCount;
            }

            if (pendingCount > pending_max_num) {
                allocator.clearPendingDeallocatedDescriptorSets();
                didClear = true;
                pendingCount = 0;
                pendingFrameCount = 0;
            }

        } else {

            if (pendingFrameCount % _maxFrameInFlight == 0) {
                allocator.clearPendingDeallocatedDescriptorSets();
            }
            ++pendingFrameCount;
        }

    }

    void clear() {
        if (cacheDescriptorSet) {
            setCache.clear();
        }
        allocator.reset_pools();

        didClear = true;
        pendingCount = 0;
        pendingFrameCount = 0;
    }


};





}

#endif//OJOIE_VULKAN_DESCRIPTOR_MANAGER_HPP
