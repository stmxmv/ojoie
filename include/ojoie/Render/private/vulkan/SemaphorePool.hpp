//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_SEMAPHOREPOOL_HPP
#define OJOIE_SEMAPHOREPOOL_HPP

#include "Render/private/vulkan.hpp"
#include "ojoie/Configuration/typedef.h"
#include <vector>

namespace AN::VK {

class Device;

/// binary semaphore pool
class SemaphorePool : private NonCopyable {
    Device *_device;
    std::vector<VkSemaphore> semaphores;
    uint32_t active_semaphore_count;
public:

    SemaphorePool() = default;

    SemaphorePool(SemaphorePool &&other) noexcept
        : _device(other._device), semaphores(std::move(other.semaphores)), active_semaphore_count(other.active_semaphore_count) {
        other.semaphores.clear();
        other.active_semaphore_count = 0;
        other._device = nullptr;
    }

    ~SemaphorePool() {
        deinit();
    }

    bool init(Device &device) {
        _device = &device;
        active_semaphore_count = 0;
        return true;
    }

    void deinit();

    void reset() {
        active_semaphore_count = 0;
    }

    VkSemaphore newSemaphore();

    uint32_t getActiveSemaphoreCount() const {
        return active_semaphore_count;
    }
};

}

#endif//OJOIE_SEMAPHOREPOOL_HPP
