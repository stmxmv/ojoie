//
// Created by Aleudillonam on 9/6/2022.
//


#include "Render/private/vulkan/SemaphorePool.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {


void SemaphorePool::deinit() {
    reset();
    // Destroy all semaphores
    for (VkSemaphore semaphore : semaphores) {
        vkDestroySemaphore(_device->vkDevice(), semaphore, nullptr);
    }

    semaphores.clear();
}

VkSemaphore SemaphorePool::newSemaphore() {
    // Check if there is an available semaphore
    if (active_semaphore_count < semaphores.size()) {
        return semaphores.at(active_semaphore_count++);
    }

    VkSemaphore semaphore{VK_NULL_HANDLE};

    VkSemaphoreCreateInfo create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkResult result = vkCreateSemaphore(_device->vkDevice(), &create_info, nullptr, &semaphore);

    if (result != VK_SUCCESS) {
        throw AN::Exception("Failed to create vulkan semaphore");
    }

    semaphores.push_back(semaphore);

    active_semaphore_count++;

    return semaphore;
}

}