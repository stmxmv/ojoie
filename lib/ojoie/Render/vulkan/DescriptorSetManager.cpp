//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/DescriptorSetManager.hpp"

#include <algorithm>

namespace AN::VK {


VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes &poolSizes, int count, VkDescriptorPoolCreateFlags flags) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());
    for (auto sz : poolSizes.sizes) {
        sizes.push_back({sz.first, uint32_t(sz.second * count)});
    }
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = flags;
    pool_info.maxSets                    = count;
    pool_info.poolSizeCount              = (uint32_t) sizes.size();
    pool_info.pPoolSizes                 = sizes.data();

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

    return descriptorPool;
}

void DescriptorAllocator::reset_pools() {
    for (auto p : usedPools) {
        vkResetDescriptorPool(device, p, 0);
    }

    freePools = usedPools;
    usedPools.clear();
    currentPool = VK_NULL_HANDLE;
    freeDescriptorSets.clear();
    pendingDeallocatedDescriptorSets.clear();
}

bool DescriptorAllocator::allocate(VkDescriptorSet *set, VkDescriptorSetLayout layout) {
    auto iter = freeDescriptorSets.find(layout);
    if (iter != freeDescriptorSets.end()) {
        *set = iter->second;
        freeDescriptorSets.erase(iter);
        return true;
    }

    if (currentPool == VK_NULL_HANDLE) {
        currentPool = grab_pool();
        usedPools.push_back(currentPool);
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext                       = nullptr;

    allocInfo.pSetLayouts        = &layout;
    allocInfo.descriptorPool     = currentPool;
    allocInfo.descriptorSetCount = 1;


    VkResult allocResult    = vkAllocateDescriptorSets(device, &allocInfo, set);
    bool     needReallocate = false;

    switch (allocResult) {
        case VK_SUCCESS:
            //all good, return
            return true;

            break;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            //reallocate pool
            needReallocate = true;
            break;
        default:
            //unrecoverable error
            return false;
    }

    if (needReallocate) {
        //allocate a new pool and retry
        currentPool = grab_pool();
        usedPools.push_back(currentPool);
        allocInfo.descriptorPool = currentPool;

        allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

        //if it still fails then we have big issues
        if (allocResult == VK_SUCCESS) {
            return true;
        }
    }

    return false;
}

void DescriptorAllocator::pendingDeallocate(VkDescriptorSet set, VkDescriptorSetLayout layout) {
    pendingDeallocatedDescriptorSets.emplace_back(layout, set);
}

void DescriptorAllocator::deallocate(VkDescriptorSet set, VkDescriptorSetLayout layout) {
    freeDescriptorSets.insert({layout, set});
}

void DescriptorAllocator::clearPendingDeallocatedDescriptorSets() {
    for (auto &[layout, set] : pendingDeallocatedDescriptorSets) {
        freeDescriptorSets.insert({layout, set});
    }
    pendingDeallocatedDescriptorSets.clear();
}

bool DescriptorAllocator::init(VkDevice newDevice) {
    device = newDevice;
    return true;
}

void DescriptorAllocator::deinit() {
    //delete every pool held
    for (auto p : freePools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    for (auto p : usedPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }

    pendingDeallocatedDescriptorSets.clear();
    freeDescriptorSets.clear();

    freePools.clear();
    usedPools.clear();
    currentPool = VK_NULL_HANDLE;
}

VkDescriptorPool DescriptorAllocator::grab_pool() {
    if (freePools.size() > 0) {
        VkDescriptorPool pool = freePools.back();
        freePools.pop_back();
        return pool;
    } else {
        return createPool(device, descriptorSizes, 1000, 0);
    }
}

bool DescriptorSetManager::init(VkDevice device) {
    _device = device;
    allocator.init(device);
    _version = 0;
    return true;
}

void DescriptorSetManager::clear() {
    setCache.clear();
    allocator.reset_pools();
}

void DescriptorSetManager::deinit() {
    allocator.deinit();
}

void DescriptorSetManager::updateDescriptorSet(const DescriptorSetInfo &info, VkDescriptorSet set) {
    std::vector<VkWriteDescriptorSet> writes = info.generateWriteDescriptorSet(set);
    vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
}

VkDescriptorSet DescriptorSetManager::getPersistentDescriptorSet(const DescriptorSetInfo &info) {
    VkDescriptorSet set;
    {
        std::lock_guard lock(spinLock);
        allocator.allocate(&set, info.layout->vkDescriptorSetLayout());
    }

    updateDescriptorSet(info, set);
    return set;
}

void DescriptorSetManager::pendingReturnPersistentDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout) {
    std::lock_guard lock(spinLock);
    allocator.pendingDeallocate(set, layout);
}

VkDescriptorSet DescriptorSetManager::descriptorSet(const DescriptorSetInfo &info) {

    CacheItem cacheItem{};
    size_t    hashValue = info.hashValue();

    bool getResult;
    {
        std::lock_guard lock(spinLock);
        getResult = setCache.get(hashValue, cacheItem);
    }

    if (getResult) {
        /// update frame index
        if (cacheItem.version != _version) {
            cacheItem.version = _version;
            std::lock_guard lock(spinLock);
            setCache.set(hashValue, cacheItem);
        }

        return cacheItem.set;
    }

    cacheItem.layout = info.layout->vkDescriptorSetLayout();
    cacheItem.version  = _version;

    {
        std::lock_guard lock(spinLock);
        allocator.allocate(&cacheItem.set, info.layout->vkDescriptorSetLayout());

        setCache.set(hashValue, cacheItem);
    }

    updateDescriptorSet(info, cacheItem.set);

    return cacheItem.set;
}

void DescriptorSetManager::update() {
    ++_version;

    /// check exceed max cache size and try to delete some
    /// slow path
    if (setCache.size() > cache_descriptor_set_threhold) {
        /// we scan the least recently used half that have different version number
        auto willDeleteView = setCache.mapLRUView() |
                              std::views::take(setCache.size() / 2) |
                              std::views::filter([this](auto &&pair) {
                                  return _version - pair.second.version > kMaxFrameInFlight + 10; // we keep a more safe area
                              });

        /// we must copy the view into vector, cause view will not work when erasing items
        std::vector<std::pair<size_t, CacheItem>> willDelete;
        willDelete.reserve(setCache.size() / 2);
        std::ranges::copy(willDeleteView, std::back_inserter(willDelete));

        for (const auto &[key, item] : willDelete) {
            allocator.deallocate(item.set, item.layout);
            setCache.erase(key);
        }
    }
}

DescriptorSetManager &GetDescriptorSetManager() {
    static DescriptorSetManager descriptorSetManager;
    return descriptorSetManager;
}

std::vector<VkWriteDescriptorSet> DescriptorSetInfo::generateWriteDescriptorSet(VkDescriptorSet dstSet) const {
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
}// namespace AN::VK