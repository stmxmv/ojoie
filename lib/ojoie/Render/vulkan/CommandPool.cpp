//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {


VkCommandBuffer CommandPool::allocateCommandBuffer(VkCommandBufferLevel level) {
    VkCommandBuffer newBuffer;
    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

    allocate_info.commandPool        = handle;
    allocate_info.commandBufferCount = 1;
    allocate_info.level              = level;

    VkResult result = vkAllocateCommandBuffers(_device->vkDevice(), &allocate_info, &newBuffer);

    if (result != VK_SUCCESS) {
        ANLog("Failed to allocate command buffer code %d", result);
        return nullptr;
    }
    return newBuffer;
}

bool CommandPool::init(Device &device, uint32_t queue_family_index, CommandBufferResetMode reset_mode) {
    _device      = &device;
    _resetMode   = reset_mode;
    _queueFamilyIndex = queue_family_index;

    VkCommandPoolCreateFlags flags;
    switch (reset_mode) {
        case CommandBufferResetMode::ResetIndividually:
            flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            break;
        case CommandBufferResetMode::ResetPool:
        default:
            flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            break;
    }

    VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

    create_info.queueFamilyIndex = queue_family_index;
    create_info.flags            = flags;

    auto result = vkCreateCommandPool(_device->vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Failed to create command pool");
        return false;
    }
    return true;
}

void CommandPool::deinit() {
    primary_command_buffers.clear();
    secondary_command_buffers.clear();

    // Destroy command pool
    if (handle != VK_NULL_HANDLE) {
        vkDestroyCommandPool(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

VkResult CommandPool::reset() {
    VkResult result;
    switch (_resetMode) {
        case CommandBufferResetMode::ResetIndividually:
        case CommandBufferResetMode::ResetPool: {
            result = vkResetCommandPool(_device->vkDevice(), handle, 0);

            if (result != VK_SUCCESS) {
                return result;
            }

            active_primary_command_buffer_count = 0;
            active_secondary_command_buffer_count = 0;
            break;
        }
        default:
            throw std::runtime_error("Unknown reset mode for command pools");
    }

    return result;
}



}

