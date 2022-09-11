//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_RC_BLITCOMMANDENCODER_HPP
#define OJOIE_RC_BLITCOMMANDENCODER_HPP

#include <ojoie/Core/typedef.h>

namespace AN::RC {

class Buffer;
class Texture;

enum class BlitCommandEncoderFlag {
    None = 0,
    ShouldFree = 1 << 0
};

class BlitCommandEncoder : private NonCopyable {

    void *impl{};
    BlitCommandEncoderFlag flag{};

public:

    BlitCommandEncoder() = default;

    BlitCommandEncoder(BlitCommandEncoder &&other) noexcept : impl(other.impl), flag(other.flag) {
        other.impl = nullptr;
        other.flag = BlitCommandEncoderFlag::None;
    }

    ~BlitCommandEncoder();

    void copyBufferToTexture(Buffer &buffer,uint64_t bufferOffset,
                             uint64_t bufferRowLength, uint64_t bufferImageHeight,
                             Texture &texture,uint32_t mipmapLevel,
                             int32_t xOffset, int32_t yOffset,
                             uint32_t width, uint32_t height);

    void copyBufferToBuffer(Buffer &srcBuffer, uint64_t srcOffset, Buffer &dstBuffer, uint64_t dstOffset, uint64_t size);

    void generateMipmapsForTexture(Texture &texture, uint32_t mipmapLevels = UINT32_MAX);

    void submit();

    void waitComplete();
};

}

namespace AN {

template<>
struct enable_bitmask_operators<RC::BlitCommandEncoderFlag> : std::true_type {};

}

#endif//OJOIE_RC_BLITCOMMANDENCODER_HPP
