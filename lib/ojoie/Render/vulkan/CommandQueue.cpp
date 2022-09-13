//
// Created by Aleudillonam on 9/12/2022.
//

#include "Render/CommandQueue.hpp"
#include "Render/private/vulkan/CommandQueue.hpp"

#include "Template/Access.hpp"

namespace AN {
namespace {

struct CommandBufferImplTag : Access::TagBase<CommandBufferImplTag> {};
struct CommandBufferShouldFreeTag : Access::TagBase<CommandBufferShouldFreeTag> {};
}

template struct Access::Accessor<CommandBufferImplTag, &RC::CommandBuffer::impl>;
template struct Access::Accessor<CommandBufferShouldFreeTag, &RC::CommandBuffer::shouldFree>;
}

namespace AN::VK {

bool CommandQueue::init(AN::VK::Device &device, const AN::VK::Queue &queue, AN::VK::CommandBufferResetMode resetMode) {
    _queue = &queue;
    return _fencePool.init(device.vkDevice()) && _commandPool.init(device, queue.getFamilyIndex(), resetMode);
}

}
namespace AN::RC {


CommandBuffer CommandQueue::commandBuffer() {
    VK::CommandQueue *self = (VK::CommandQueue *)impl;

    CommandBuffer ret{};

    Access::set<CommandBufferImplTag>(ret, new VK::CommandBuffer(self->commandBuffer()));
    Access::set<CommandBufferShouldFreeTag>(ret, true);

    return ret;
}

void CommandQueue::reset() {
    VK::CommandQueue *self = (VK::CommandQueue *)impl;
    self->reset();
}


}