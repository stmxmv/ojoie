//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_VERTEXBUFFER_HPP
#define OJOIE_VERTEXBUFFER_HPP

namespace AN {
struct RenderContext;
}
namespace AN::RC {



class VertexBuffer : private NonCopyable {

    struct Impl;
    Impl *impl;

    bool initCommon();
public:

    VertexBuffer();

    VertexBuffer(VertexBuffer &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    VertexBuffer &operator = (VertexBuffer &&other) noexcept;

    ~VertexBuffer();

    bool initStatic(void *vertices, uint64_t bytes);

    bool initDynamic(uint64_t bytes, bool writeOnly = true);

    /// \brief only available when initDynamic called
    void *content();

    void deinit();

    void bind(uint64_t offset = 0);

};




}

#endif//OJOIE_VERTEXBUFFER_HPP
