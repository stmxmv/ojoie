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
#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/BufferPool.hpp"

namespace AN::VK {


class RenderFrame {

    FencePool fence_pool;

    SemaphorePool semaphore_pool;

    RenderTarget _renderTarget;

    DescriptorSetManager descriptorSetManager;

    /// Commands pools associated to the frame
    std::map<uint32_t, CommandPool> command_pools;

    BufferPool vertexBufferPool;
    BufferPool indexBufferPool;
    BufferPool stageBufferPool;

    Device *_device;

    CommandPool &get_command_pools(uint32_t familyIndex, CommandBufferResetMode reset_mode);

public:

    RenderFrame() = default;

    RenderFrame(RenderFrame &&other) noexcept
        : fence_pool(std::move(other.fence_pool)), semaphore_pool(std::move(other.semaphore_pool)),
          _renderTarget(std::move(other._renderTarget)), descriptorSetManager(std::move(other.descriptorSetManager)),
          vertexBufferPool(std::move(other.vertexBufferPool)),
          indexBufferPool(std::move(other.indexBufferPool)),
          stageBufferPool(std::move(other.stageBufferPool)),
          _device(other._device) {



    }

    ~RenderFrame() {
        deinit();
    }

    bool init(Device &device, RenderTarget &&render_target);

    void deinit() {
        vertexBufferPool.deinit();
        indexBufferPool.deinit();
        stageBufferPool.deinit();
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

        vertexBufferPool.reset();
        indexBufferPool.reset();
        stageBufferPool.reset();
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

    CommandBuffer commandBuffer(const Queue &queue,
                                CommandBufferResetMode reset_mode = CommandBufferResetMode::ResetPool,
                                VkCommandBufferLevel level        = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {

        CommandPool &command_pool = get_command_pools(queue.getFamilyIndex(), reset_mode);
        return { queue, command_pool.newCommandBuffer(level), reset_mode, fence() };
    }


    BufferPool &getVertexBufferPool() {
        return vertexBufferPool;
    }

    BufferPool &getIndexBufferPool() {
        return indexBufferPool;
    }

    BufferPool &getStageBufferPool() {
        return stageBufferPool;
    }
};


}

#endif//OJOIE_RENDERFRAME_HPP
