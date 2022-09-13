//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_COMMANDBUFFER_HPP
#define OJOIE_COMMANDBUFFER_HPP

#include <ojoie/Render/BlitCommandEncoder.hpp>

namespace AN::RC {


/// \brief bridge to VK::CommandBuffer in vulkan
class CommandBuffer : private NonCopyable {
    void *impl{};
    bool shouldFree{};
public:

    CommandBuffer() = default;

    CommandBuffer(CommandBuffer &&other) noexcept : impl(other.impl), shouldFree(other.shouldFree) {
        other.impl = nullptr;
        other.shouldFree = false;
    }

    ~CommandBuffer();

    BlitCommandEncoder blitCommandEncoder();

    /// \brief submit to the associated queue, cannot be called concurrently
    /// \attention submit multiple times is not yet supported
    void submit();

    /// \brief wait single command buffer to complete
    void waitUntilCompleted();

    /// \brief reset the commandBuffer, so that it can be resubmitted, only valid if commandBuffer allocated with flag ResetIndividually
    void reset();
};

}

#endif//OJOIE_COMMANDBUFFER_HPP
