//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_VERTEXBUFFER_HPP
#define OJOIE_VERTEXBUFFER_HPP

#include <ojoie/Render/Buffer.hpp>

namespace AN {
struct RenderContext;
}

namespace AN::VK {
class RenderCommandEncoder;
}

namespace AN::RC {



class VertexBuffer : private NonCopyable {

    RC::Buffer vertexBuffer;

    /// dynamic
    uint32_t maxFrameInFlight{};
    uint32_t frameCount;
    uint64_t padSize;
    void *mappedBuffer;
    bool _writeOnly;

#ifdef OJOIE_USE_VULKAN

    friend class VK::RenderCommandEncoder;
#endif

public:

    VertexBuffer() = default;

    VertexBuffer(VertexBuffer &&other) noexcept
        : vertexBuffer(std::move(other.vertexBuffer)), maxFrameInFlight(other.maxFrameInFlight),
          frameCount(other.frameCount), padSize(other.padSize), mappedBuffer(other.mappedBuffer), _writeOnly(other._writeOnly) {
        other.maxFrameInFlight = 0;
        other.frameCount = 0;
        other.padSize = 0;
        other.mappedBuffer = nullptr;
        other._writeOnly = false;
    }

    VertexBuffer &operator = (VertexBuffer &&other) noexcept {
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    ~VertexBuffer() {
        deinit();
    }

    bool init(uint64_t bytes);

    bool initDynamic(uint64_t bytes, bool writeOnly = true);

    RC::Buffer &getBuffer() {
        return vertexBuffer;
    }

    /// \brief only available when initDynamic called
    void *content();

    void deinit();

    uint64_t getBufferOffset(uint64_t offset);
};




}

#endif//OJOIE_VERTEXBUFFER_HPP
