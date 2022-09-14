//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/RenderFrame.hpp"
#include "Render/private/vulkan/Device.hpp"

#define BUFFER_POOL_BLOCK_SIZE (256 * 1024)

namespace AN::VK {

bool RenderFrame::init(Device &device, RenderTarget &&render_target) {
    _device = &device;

    std::construct_at(&_renderTarget, std::move(render_target));

    VkBufferUsageFlagBits supportedUsageFlags[] = {
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };

    return fence_pool.init(device.vkDevice()) && semaphore_pool.init(device) && descriptorSetManager.init(device.vkDevice()) &&
            bufferManager.init(device, supportedUsageFlags, BUFFER_POOL_BLOCK_SIZE);
}

CommandPool &RenderFrame::get_command_pools(uint32_t familyIndex, CommandBufferResetMode reset_mode) {
    auto command_pool_it = command_pools.find(familyIndex);

    if (command_pool_it != command_pools.end()) {
        if (command_pool_it->second.getResetMode() != reset_mode) {
            _device->waitIdle();

            // Delete pools
            command_pools.erase(command_pool_it);
        } else {
            return command_pool_it->second;
        }
    }

    CommandPool commandPool;
    if (!commandPool.init(*_device, familyIndex, reset_mode)) {
        throw std::runtime_error("Failed to init vulkan command pool");
    }

    auto res_ins_it = command_pools.emplace(familyIndex, std::move(commandPool));

    command_pool_it = res_ins_it.first;

    return command_pool_it->second;


}


}