//
// Created by Aleudillonam on 8/13/2022.
//

#ifndef OJOIE_UNIFORMBUFFER_HPP
#define OJOIE_UNIFORMBUFFER_HPP

#include <ojoie/Render/Buffer.hpp>

namespace AN {
struct RenderContext;
class Renderer;
}

#ifdef OJOIE_USE_VULKAN
namespace AN::VK {
class RenderCommandEncoder;
}
#endif

namespace AN::RC {

/// \deprecated prefer BufferManager::buffer, ether through renderContext param or GetRenderer().getRenderContext()
///             to get the bufferManager to subAllocate uniformBuffer from a large buffer
class UniformBuffer : private NonCopyable {

    RC::Buffer uniformBuffer;
    uint64_t bufferSize;
    uint64_t padSize;

    uint32_t frameCount;
    uint32_t maxFrameInFlight;

    void *mappedBuffer;
    bool _writeOnly;

    uint64_t _size;



    friend class RenderPipeline;
    friend class AN::Renderer;
#ifdef OJOIE_USE_VULKAN
    friend class AN::VK::RenderCommandEncoder;
#endif
public:

    UniformBuffer() = default;

    UniformBuffer(UniformBuffer &&other) noexcept
        : uniformBuffer(std::move(other.uniformBuffer)), bufferSize(other.bufferSize), padSize(other.padSize),
          frameCount(other.frameCount), maxFrameInFlight(other.maxFrameInFlight), mappedBuffer(other.mappedBuffer),
          _writeOnly(other._writeOnly), _size(other._size) {
        other.bufferSize = 0;
        other.padSize = 0;
        other.frameCount = 0;
        other.maxFrameInFlight = 0;
        other.mappedBuffer = nullptr;
        other._writeOnly = false;
        other._size = 0;
    }

    UniformBuffer &operator = (UniformBuffer &&other) noexcept {
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    ~UniformBuffer() {
        deinit();
    }

    RC::Buffer &getBuffer() {
        return uniformBuffer;
    }

    bool init(uint64_t size, bool writeOnly = true);

    void deinit();

    UniformBuffer copy() const;

    void *content();

    uint64_t getSize() const {
        return _size;
    }

    uint32_t getOffset();
};


}

#endif//OJOIE_UNIFORMBUFFER_HPP
