//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/BlitCommandEncoder.hpp"
#include "Render/private/vulkan/SemaphorePool.hpp"
#include "Render/CommandBuffer.hpp"

namespace AN::VK {



}
namespace AN::RC {

CommandBuffer::~CommandBuffer() {
    if (shouldFree && impl) {
        delete (VK::CommandBuffer *)impl;
        impl = nullptr;
    }
}

BlitCommandEncoder CommandBuffer::blitCommandEncoder() {
    VK::CommandBuffer *self = (VK::CommandBuffer *)impl;
    VK::BlitCommandEncoder *blitCommandEncoder = new VK::BlitCommandEncoder(self->blitCommandEncoder());
    BlitCommandEncoder ret{};

    *(VK::BlitCommandEncoder **)&ret = blitCommandEncoder;

    bool *encoderShouldFree = (bool *)((char *)&ret + sizeof(void *));
    *encoderShouldFree = true;

    return ret;
}

void CommandBuffer::submit() {
    VK::CommandBuffer *self = (VK::CommandBuffer *)impl;
    self->submit();
}

void CommandBuffer::waitUntilCompleted() {
    VK::CommandBuffer *self = (VK::CommandBuffer *)impl;
    self->waitUntilCompleted();
}

void CommandBuffer::reset() {
    VK::CommandBuffer *self = (VK::CommandBuffer *)impl;
    self->reset();
}

}

