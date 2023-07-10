//
// Created by Aleudillonam on 8/28/2022.
//

#ifndef OJOIE_FENCEPOOL_HPP
#define OJOIE_FENCEPOOL_HPP

#include "ojoie/Render/private/vulkan.hpp"
#include "ojoie/Configuration/typedef.h"

#include <ojoie/Render/private/vulkan.hpp>
#include <vector>

namespace AN::VK {

/// abstraction of VkFence that can reset and reuse allocated VkFence
class FencePool : private NonCopyable {
    VkDevice _device{};
    std::vector<VkFence> fences;
    uint32_t active_fence_count;

public:

    FencePool() = default;

    FencePool(FencePool &&other) noexcept
        : _device(other._device), fences(std::move(other.fences)), active_fence_count(other.active_fence_count) {
        other._device = nullptr;
    }

    ~FencePool() {
        deinit();
    }

    bool init(VkDevice device) {
        _device = device;
        active_fence_count = 0;
        return true;
    }

    void deinit() {
        if (_device) {
            wait();
            reset();

            // Destroy all fences
            for (VkFence fence : fences) {
                vkDestroyFence(_device, fence, nullptr);
            }

            fences.clear();

            _device = nullptr;
        }
    }

    VkFence newFence() {
        if (active_fence_count < fences.size())
        {
            return fences.at(active_fence_count++);
        }

        VkFence fence{VK_NULL_HANDLE};

        VkFenceCreateInfo create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        VkResult result = vkCreateFence(_device, &create_info, nullptr, &fence);

        if (result != VK_SUCCESS) {
            throw VK::Exception(result, "Failed to create fence.");
        }

        fences.push_back(fence);

        active_fence_count++;

        return fences.back();
    }

    VkResult wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const {
        if (active_fence_count < 1 || fences.empty()) {
            return VK_SUCCESS;
        }

        return vkWaitForFences(_device, active_fence_count, fences.data(), true, timeout);
    }

    VkResult waitOne(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const {
        if (active_fence_count < 1 || fences.empty()) {
            return VK_SUCCESS;
        }

        return vkWaitForFences(_device, active_fence_count, fences.data(), false, timeout);
    }

    VkResult reset() {
        if (active_fence_count < 1 || fences.empty())
        {
            return VK_SUCCESS;
        }

        VkResult result = vkResetFences(_device, active_fence_count, fences.data());

        if (result != VK_SUCCESS)
        {
            return result;
        }

        active_fence_count = 0;

        return VK_SUCCESS;
    }


};



}

#endif//OJOIE_FENCEPOOL_HPP
