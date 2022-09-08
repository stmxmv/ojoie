//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_RENDERFRAME_HPP
#define OJOIE_RENDERFRAME_HPP

#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/SemaphorePool.hpp"
#include "Render/private/vulkan/DescriptorSetManager.hpp"
#include "Render/private/vulkan/FencePool.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/Queue.hpp"

namespace AN::VK {


class RenderFrame {

    FencePool fence_pool;

    SemaphorePool semaphore_pool;

    RenderTarget _renderTarget;

    DescriptorSetManager descriptorSetManager;

    /// Commands pools associated to the frame
    std::map<uint32_t, CommandPool> command_pools;

    Device *_device;

    CommandPool &get_command_pools(const Queue &queue, CommandBufferResetMode reset_mode);

public:

    RenderFrame() = default;

    RenderFrame(RenderFrame &&other) noexcept
        : fence_pool(std::move(other.fence_pool)), semaphore_pool(std::move(other.semaphore_pool)),
          _renderTarget(std::move(other._renderTarget)), descriptorSetManager(std::move(other.descriptorSetManager)),
          _device(other._device) {



    }

    ~RenderFrame() {
        deinit();
    }

    bool init(Device &device, RenderTarget &&render_target);

    void deinit() {
        fence_pool.deinit();
        semaphore_pool.deinit();
        descriptorSetManager.deinit();
        command_pools.clear();
        _renderTarget.clear();
    }

    void reset(bool fence = true) {
        if (fence) {
            if (VkResult result = fence_pool.wait(); result != VK_SUCCESS) {
                ANLog("fence_pool.wait() return %s", ResultCString(result));
            }

            fence_pool.reset();
        }

        for (auto &command_pools_per_queue : command_pools) {
            command_pools_per_queue.second.reset();
        }

        semaphore_pool.reset();
    }

    const RenderTarget &getRenderTarget() const {
        return _renderTarget;
    }

    void replaceRenderTarget(RenderTarget &&renderTarget) {
        _renderTarget.clear();
        std::destroy_at(&_renderTarget);
        std::construct_at(&_renderTarget, std::move(renderTarget));
    }

    VkDescriptorSet descriptorSet(const DescriptorSetInfo &info) {
        return descriptorSetManager.descriptorSet(info);
    }

    DescriptorSetManager &getDescriptorSetManager() {
        return descriptorSetManager;
    }

    void clearDescriptorSet() {
        descriptorSetManager.clear();
    }

    VkSemaphore semaphore() {
        return semaphore_pool.newSemaphore();
    }

    VkFence fence() {
        return fence_pool.newFence();
    }

    VkCommandBuffer commandBuffer(const Queue &queue,
                                  CommandBufferResetMode reset_mode = CommandBufferResetMode::ResetPool,
                                  VkCommandBufferLevel level        = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        auto &command_pool = get_command_pools(queue, reset_mode);

        return command_pool.newCommandBuffer(level);
    }


};


}

#endif//OJOIE_RENDERFRAME_HPP
