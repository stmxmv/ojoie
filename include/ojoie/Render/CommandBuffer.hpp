//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_COMMANDBUFFER_HPP
#define OJOIE_COMMANDBUFFER_HPP

#include <ojoie/Render/BlitCommandEncoder.hpp>

namespace AN::RC {

/// \brief actually VK::CommandPool in vulkan
class CommandBuffer {
    void *impl{};
public:

    BlitCommandEncoder blitCommandEncoder();

};

}

#endif//OJOIE_COMMANDBUFFER_HPP
