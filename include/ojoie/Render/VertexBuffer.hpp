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

    ~VertexBuffer();

    bool initStatic(void *vertices, uint64_t bytes);

    bool initDynamic(uint64_t bytes);

    /// \brief only available when initDynamic called
    void *mapMemory();

    /// \brief only available when initDynamic called
    void unMapMemory();

    void deinit();

    void bind(uint64_t offset = 0);

};




}

#endif//OJOIE_VERTEXBUFFER_HPP
