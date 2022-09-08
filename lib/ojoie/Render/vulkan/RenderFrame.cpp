//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/RenderFrame.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

bool RenderFrame::init(Device &device, RenderTarget &&render_target) {
    _device = &device;

    std::construct_at(&_renderTarget, std::move(render_target));

    return fence_pool.init(device.vkDevice()) && semaphore_pool.init(device) && descriptorSetManager.init(device.vkDevice());
}

CommandPool &RenderFrame::get_command_pools(const Queue &queue, CommandBufferResetMode reset_mode) {
    auto command_pool_it = command_pools.find(queue.getFamilyIndex());

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
    if (!commandPool.init(_device->vkDevice(), queue.getFamilyIndex(), reset_mode)) {
        throw std::runtime_error("Failed to init vulkan command pool");
    }

    auto res_ins_it = command_pools.emplace(queue.getFamilyIndex(), std::move(commandPool));

    command_pool_it = res_ins_it.first;

    return command_pool_it->second;


}


}