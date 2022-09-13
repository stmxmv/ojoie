//
// Created by Aleudillonam on 9/9/2022.
//

#ifndef OJOIE_RC_RENDERCOMMANDENCODER_HPP
#define OJOIE_RC_RENDERCOMMANDENCODER_HPP

#include <ojoie/Core/typedef.h>
#include <ojoie/Render/RenderPipeline.hpp>

namespace AN::RC {

class UniformBuffer;
class Texture;
class Sampler;
class IndexBuffer;
class VertexBuffer;
class Buffer;

enum class IndexType;

struct Viewport {
    float originX, originY;
    float width, height;
    float znear, zfar;
};

struct ScissorRect {
    int32_t x, y;
    int32_t width, height;
};

enum class CullMode;

/// \brief RenderCommandEncoder is just a interface class,
///        can only obtain it from commandBuffer or renderContext
class RenderCommandEncoder : private NonCopyable {

    void *impl;

public:

    void setViewport(const Viewport &viewport);

    void setScissor(const ScissorRect &scissorRect);

    void setCullMode(CullMode cullMode);

    void bindRenderPipeline(class RC::RenderPipeline &renderPipeline);

    void bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, class RC::Buffer &uniformBuffer);

    void bindTexture(uint32_t binding, class RC::Texture &texture);

    void bindSampler(uint32_t binding, class RC::Sampler &sampler);

    void bindIndexBuffer(RC::IndexType type, uint64_t offset, RC::Buffer &indexBuffer);

    void bindVertexBuffer(uint32_t binding, uint64_t offset, Buffer &vertexBuffer);

    void bindVertexBuffer(uint32_t binding, uint32_t bindingCount, const uint64_t *offset, Buffer *const *vertexBuffer);

    void pushConstants(RC::ShaderStageFlag stageFlag, uint32_t offset, uint32_t size, const void *data);

    void drawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceCount = 1, uint32_t instanceOffset = 0);

    void draw(uint32_t count);

};

}

namespace AN {

template<>
struct enable_bitmask_operators<RC::CullMode> : std::true_type {};

}

#endif//OJOIE_RC_RENDERCOMMANDENCODER_HPP
