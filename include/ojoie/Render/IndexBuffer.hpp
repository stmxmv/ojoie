//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_INDEXBUFFER_HPP
#define OJOIE_INDEXBUFFER_HPP

namespace AN {
struct RenderContext;
}

namespace AN::RC {

enum class IndexType {
    UInt32,
    UInt16
};

class IndexBuffer : private NonCopyable {


    struct Impl;
    Impl *impl;
public:

    IndexBuffer();

    ~IndexBuffer();

    IndexBuffer(IndexBuffer &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    bool initStatic(void *indices, uint64_t bytes);

    void deinit();

    /// \param offset element offset
    void bind(IndexType type, uint64_t offset = 0);

};




}

#endif//OJOIE_INDEXBUFFER_HPP
