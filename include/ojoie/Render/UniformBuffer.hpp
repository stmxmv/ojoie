//
// Created by Aleudillonam on 8/13/2022.
//

#ifndef OJOIE_UNIFORMBUFFER_HPP
#define OJOIE_UNIFORMBUFFER_HPP

namespace AN {
struct RenderContext;
class Renderer;
}
namespace AN::RC {


class UniformBuffer : private NonCopyable {

    struct Impl;
    Impl *impl;

    uint64_t _size;

    uint32_t getOffset();

    void *getUnderlyingBuffer();

    friend class RenderPipeline;
    friend class AN::Renderer;
public:

    UniformBuffer();

    UniformBuffer(UniformBuffer &&other) noexcept :  impl(other.impl), _size(other._size) {
        other.impl = nullptr;
    }

    UniformBuffer &operator = (UniformBuffer &&other) noexcept;

    ~UniformBuffer();

    bool init(uint64_t size, bool writeOnly = true);

    void deinit();

    UniformBuffer copy() const;

    void *content();

    uint64_t getSize() const {
        return _size;
    }
};


}

#endif//OJOIE_UNIFORMBUFFER_HPP
