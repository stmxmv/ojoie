//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/DescriptorSetManager.hpp"

#include <algorithm>

namespace AN::VK {


VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
{
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());
    for (auto sz : poolSizes.sizes) {
        sizes.push_back({ sz.first, uint32_t(sz.second * count) });
    }
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = flags;
    pool_info.maxSets = count;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

    return descriptorPool;
}

void DescriptorAllocator::reset_pools()
{
    for (auto p : usedPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }

    freePools = usedPools;
    usedPools.clear();
    currentPool = VK_NULL_HANDLE;
    freeDescriptorSets.clear();
    pendingDeallocatedDescriptorSets.clear();
}

bool DescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
    auto iter = freeDescriptorSets.find(layout);
    if (iter != freeDescriptorSets.end()) {
        *set = iter->second;
        freeDescriptorSets.erase(iter);
        return true;
    }

    if (currentPool == VK_NULL_HANDLE)
    {
        currentPool = grab_pool();
        usedPools.push_back(currentPool);
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;

    allocInfo.pSetLayouts = &layout;
    allocInfo.descriptorPool = currentPool;
    allocInfo.descriptorSetCount = 1;


    VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
    bool needReallocate = false;

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

    if (needReallocate)
    {
        //allocate a new pool and retry
        currentPool = grab_pool();
        usedPools.push_back(currentPool);
        allocInfo.descriptorPool = currentPool;

        allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

        //if it still fails then we have big issues
        if (allocResult == VK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

void DescriptorAllocator::pendingDeallocate(VkDescriptorSet set, VkDescriptorSetLayout layout) {
    pendingDeallocatedDescriptorSets.emplace_back(layout, set);
}

void DescriptorAllocator::deallocate(VkDescriptorSet set, VkDescriptorSetLayout layout) {
    freeDescriptorSets.insert({ layout, set });
}

void DescriptorAllocator::clearPendingDeallocatedDescriptorSets() {
    for (auto &[layout, set] : pendingDeallocatedDescriptorSets) {
        freeDescriptorSets.insert({ layout, set });
    }
    pendingDeallocatedDescriptorSets.clear();
}

bool DescriptorAllocator::init(VkDevice newDevice) {
    device = newDevice;
    return true;
}

void DescriptorAllocator::deinit()
{
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

VkDescriptorPool DescriptorAllocator::grab_pool()
{
    if (freePools.size() > 0)
    {
        VkDescriptorPool pool = freePools.back();
        freePools.pop_back();
        return pool;
    }
    else {
        return createPool(device, descriptorSizes, 1000, 0);
    }
}





}