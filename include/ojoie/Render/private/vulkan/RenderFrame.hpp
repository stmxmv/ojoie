//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_RENDERFRAME_HPP
#define OJOIE_RENDERFRAME_HPP

#include "Render/private/vulkan/BufferManager.hpp"
#include "Render/private/vulkan/BufferPool.hpp"
#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/DescriptorSetManager.hpp"
#include "Render/private/vulkan/FencePool.hpp"
#include "Render/private/vulkan/Queue.hpp"
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/SemaphorePool.hpp"

namespace AN::VK {


class RenderFrame {
    Device *_device;

    SemaphorePool semaphore_pool;

public:
    RenderFrame() = default;

    RenderFrame(RenderFrame &&other) noexcept;

    ~RenderFrame() {
        deinit();
    }

    bool init(Device &device);

    void deinit();

    void reset(bool fence = true);

    VkSemaphore semaphore() {
        return semaphore_pool.newSemaphore();
    }

};


}// namespace AN::VK

#endif//OJOIE_RENDERFRAME_HPP
