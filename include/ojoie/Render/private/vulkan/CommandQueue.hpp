//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_VK_COMMANDQUEUE_HPP
#define OJOIE_VK_COMMANDQUEUE_HPP

#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/FencePool.hpp"
#include "Render/private/vulkan/Queue.hpp"

#include "Core/SpinLock.hpp"

namespace AN::VK {

/// \brief command queue is a combination of commandPool of a specific device's queue and fencePool,
class CommandQueue {
    const Queue *_queue;
    FencePool _fencePool;
    CommandPool _commandPool;
    bool queueActive{};
public:

    CommandQueue() = default;

    ~CommandQueue() {
        deinit();
    }

    bool init(Device &device, const Queue &queue, CommandBufferResetMode resetMode = CommandBufferResetMode::ResetPool);

    void deinit() {
        _fencePool.deinit();
        _commandPool.deinit();
    }

    CommandBuffer commandBuffer() {
        queueActive = true;
        return { *_queue, _commandPool.newCommandBuffer(), _commandPool.getResetMode(), _fencePool.newFence() };
    }

    void reset() {
        if (queueActive) {
            _fencePool.reset();
            _commandPool.reset();
            queueActive = false;
        }
    }

};

}


#endif//OJOIE_VK_COMMANDQUEUE_HPP
