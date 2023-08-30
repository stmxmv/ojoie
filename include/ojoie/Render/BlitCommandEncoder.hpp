//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_RC_BLITCOMMANDENCODER_HPP
#define OJOIE_RC_BLITCOMMANDENCODER_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Render/CommandEncoder.hpp>

namespace AN::RC {

class Buffer;
class Texture;


class BlitCommandEncoder : private NonCopyable {

    void *impl{};
    bool shouldFree{};

public:

    BlitCommandEncoder() = default;

    BlitCommandEncoder(BlitCommandEncoder &&other) noexcept : impl(other.impl), shouldFree(other.shouldFree) {
        other.impl = nullptr;
        other.shouldFree = false;
    }

    ~BlitCommandEncoder();

    void copyBufferToTexture(Buffer &buffer,uint64_t bufferOffset,
                             uint64_t bufferRowLength, uint64_t bufferImageHeight,
                             Texture &texture,uint32_t mipmapLevel,
                             int32_t xOffset, int32_t yOffset,
                             uint32_t width, uint32_t height);

    void copyBufferToBuffer(Buffer &srcBuffer, uint64_t srcOffset, Buffer &dstBuffer, uint64_t dstOffset, uint64_t size);

    void generateMipmapsForTexture(Texture &texture, uint32_t mipmapLevels = UINT32_MAX);

    void bufferMemoryBarrier(const Buffer &buffer, const BufferMemoryBarrier &memoryBarrier);

};

}


#endif//OJOIE_RC_BLITCOMMANDENCODER_HPP
