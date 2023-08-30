//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_INDEXBUFFER_HPP
#define OJOIE_INDEXBUFFER_HPP

#include <ojoie/Render/Buffer.hpp>

namespace AN {
struct RenderContext;
}

namespace AN::VK {
class RenderCommandEncoder;
}
namespace AN::RC {

enum class IndexType {
    UInt32,
    UInt16
};

class IndexBuffer : private NonCopyable {

    RC::Buffer indexBuffer;

    /// dynamic
    uint32_t maxFrameInFlight{};
    uint32_t frameCount;
    uint64_t padSize;
    void *mappedBuffer;
    bool _writeOnly;

public:

    IndexBuffer() = default;

    ~IndexBuffer() {
        deinit();
    }

    IndexBuffer(IndexBuffer &&other) noexcept : indexBuffer(std::move(other.indexBuffer)) {

    }

    bool init(uint64_t bytes);

    bool initDynamic(uint64_t bytes, bool writeOnly = true);

    void deinit();

    /// \brief only available when initDynamic called
    void *content();

    uint64_t getBufferOffset(uint64_t offset);

    RC::Buffer &getBuffer() {
        return indexBuffer;
    }

};




}

#endif//OJOIE_INDEXBUFFER_HPP
