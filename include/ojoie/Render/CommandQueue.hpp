//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_RC_COMMANDPOOL_HPP
#define OJOIE_RC_COMMANDPOOL_HPP

#include <ojoie/Core/typedef.h>
#include <ojoie/Render/CommandBuffer.hpp>

namespace AN::RC {

/// \brief bridge to VK::CommandQueue in vulkan
class CommandQueue : private NonCopyable {
    void *impl{};
public:

    CommandBuffer commandBuffer();

    void reset();

};

}

#endif//OJOIE_RC_COMMANDPOOL_HPP
