//
// Created by Aleudillonam on 3/23/2022.
//

#include "Render/private/vulkan.hpp"
#include <algorithm>

namespace AN::VK {

const char *ResultCString(VkResult result) {
#define WRITE_VK_ENUM(r) \
	case VK_##r:         \
		return #r;        \

    switch (result) {
        WRITE_VK_ENUM(NOT_READY);
        WRITE_VK_ENUM(TIMEOUT);
        WRITE_VK_ENUM(EVENT_SET);
        WRITE_VK_ENUM(EVENT_RESET);
        WRITE_VK_ENUM(INCOMPLETE);
        WRITE_VK_ENUM(ERROR_OUT_OF_HOST_MEMORY);
        WRITE_VK_ENUM(ERROR_OUT_OF_DEVICE_MEMORY);
        WRITE_VK_ENUM(ERROR_INITIALIZATION_FAILED);
        WRITE_VK_ENUM(ERROR_DEVICE_LOST);
        WRITE_VK_ENUM(ERROR_MEMORY_MAP_FAILED);
        WRITE_VK_ENUM(ERROR_LAYER_NOT_PRESENT);
        WRITE_VK_ENUM(ERROR_EXTENSION_NOT_PRESENT);
        WRITE_VK_ENUM(ERROR_FEATURE_NOT_PRESENT);
        WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DRIVER);
        WRITE_VK_ENUM(ERROR_TOO_MANY_OBJECTS);
        WRITE_VK_ENUM(ERROR_FORMAT_NOT_SUPPORTED);
        WRITE_VK_ENUM(ERROR_SURFACE_LOST_KHR);
        WRITE_VK_ENUM(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        WRITE_VK_ENUM(SUBOPTIMAL_KHR);
        WRITE_VK_ENUM(ERROR_OUT_OF_DATE_KHR);
        WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        WRITE_VK_ENUM(ERROR_VALIDATION_FAILED_EXT);
        WRITE_VK_ENUM(ERROR_INVALID_SHADER_NV);
        default:
            return "UNKNOWN_ERROR";
    }

#undef WRITE_VK_ENUM
}

void DescriptorLayoutCache::init(VkDevice newDevice)
{
    device = newDevice;
}

VkDescriptorSetLayout DescriptorLayoutCache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info)
{
    DescriptorLayoutInfo layoutinfo;
    layoutinfo.bindings.reserve(info->bindingCount);
    bool isSorted = true;
    int32_t lastBinding = -1;
    for (uint32_t i = 0; i < info->bindingCount; i++) {
        layoutinfo.bindings.push_back(info->pBindings[i]);

        //check that the bindings are in strict increasing order
        if (static_cast<int32_t>(info->pBindings[i].binding) > lastBinding)
        {
            lastBinding = info->pBindings[i].binding;
        }
        else{
            isSorted = false;
        }
    }
    if (!isSorted)
    {
        std::sort(layoutinfo.bindings.begin(), layoutinfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ) {
            return a.binding < b.binding;
        });
    }

    auto it = layoutCache.find(layoutinfo);
    if (it != layoutCache.end())
    {
        return (*it).second;
    }
    else {
        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

        //layoutCache.emplace()
        //add to cache
        layoutCache[layoutinfo] = layout;
        return layout;
    }
}


void DescriptorLayoutCache::deinit()
{
    //delete every descriptor layout held
    for (const auto& pair : layoutCache)
    {
        vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
    }
    layoutCache.clear();
}


bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
    if (other.bindings.size() != bindings.size())
    {
        return false;
    }
    else {
        //compare each of the bindings is the same. Bindings are sorted so they will match
        for (int i = 0; i < bindings.size(); i++) {
            if (other.bindings[i].binding != bindings[i].binding)
            {
                return false;
            }
            if (other.bindings[i].descriptorType != bindings[i].descriptorType)
            {
                return false;
            }
            if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
            {
                return false;
            }
            if (other.bindings[i].stageFlags != bindings[i].stageFlags)
            {
                return false;
            }
        }
        return true;
    }
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const
{
    using std::size_t;
    using std::hash;

    size_t result = hash<size_t>()(bindings.size());

    for (const VkDescriptorSetLayoutBinding& b : bindings)
    {
        //pack the binding data into a single int64. Not fully correct but its ok
        size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        //shuffle the packed binding data and xor it with the main hash
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}


}