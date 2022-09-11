//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/BlitCommandEncoder.hpp"
#include "Render/CommandBuffer.hpp"

namespace AN::RC {


BlitCommandEncoder CommandBuffer::blitCommandEncoder() {
    VK::CommandPool *self = (VK::CommandPool *)impl;
    VK::BlitCommandEncoder *blitCommandEncoder =
            new VK::BlitCommandEncoder(self->getDevice(),
                                       self->newCommandBuffer(),
                                       self->getDevice().queue(VK_QUEUE_GRAPHICS_BIT,
                                                               self->getQueueFamilyIndex()));
    BlitCommandEncoder ret{};

    *(VK::BlitCommandEncoder **)&ret = blitCommandEncoder;

    BlitCommandEncoderFlag *flag = (BlitCommandEncoderFlag *)((char *)&ret + sizeof(void *));
    *flag = BlitCommandEncoderFlag::ShouldFree;

    return ret;
}

}