//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_INDEXBUFFER_HPP
#define OJOIE_INDEXBUFFER_HPP

namespace AN {
struct RenderContext;
}

namespace AN::RC {

class IndexBuffer : private NonCopyable {


    struct Impl;
    Impl *impl;
public:

    IndexBuffer();

    ~IndexBuffer();

    IndexBuffer(IndexBuffer &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    bool initStatic(uint32_t *indices, uint64_t size);

    void deinit();

    void bind(uint64_t offset = 0);

};




}

#endif//OJOIE_INDEXBUFFER_HPP
