//
// Created by Aleudillonam on 9/9/2022.
//

#include "Render/RenderCommandEncoder.hpp"
#include "Render/private/vulkan/RenderCommandEncoder.hpp"

#include "Template/Access.hpp"


namespace AN {

namespace  {

struct BufferImplTag : Access::TagBase<BufferImplTag> {};

}

template struct Access::Accessor<BufferImplTag, &RC::Buffer::impl>;

}

namespace AN::RC {


void RenderCommandEncoder::setViewport(const Viewport &viewport) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->setViewport(viewport);
}
void RenderCommandEncoder::setScissor(const ScissorRect &scissorRect) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->setScissor(scissorRect);
}

void RenderCommandEncoder::setCullMode(CullMode cullMode) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->setCullMode(cullMode);
}

void RenderCommandEncoder::bindRenderPipeline(RenderPipeline &renderPipeline) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindRenderPipeline(renderPipeline);
}

void RenderCommandEncoder::bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, Buffer &uniformBuffer) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindUniformBuffer(binding, offset, size, *(VK::Buffer *)Access::get<BufferImplTag>(uniformBuffer));
}

void RenderCommandEncoder::bindTexture(uint32_t binding, Texture &texture) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindImageView(binding, texture.getImageView());
}

void RenderCommandEncoder::bindSampler(uint32_t binding, Sampler &sampler) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindSampler(binding, sampler);
}

void RenderCommandEncoder::drawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceCount, uint32_t instanceOffset) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->drawIndexed(indexCount, indexOffset, vertexOffset, instanceCount, instanceOffset);
}

void RenderCommandEncoder::draw(uint32_t count) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->draw(count);
}

void RenderCommandEncoder::pushConstants(RC::ShaderStageFlag stageFlag, uint32_t offset, uint32_t size, const void *data) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->pushConstants(stageFlag, offset, size, data);
}

void RenderCommandEncoder::bindIndexBuffer(RC::IndexType type, uint64_t offset, Buffer &indexBuffer) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindIndexBuffer(type, offset, *(VK::Buffer *)Access::get<BufferImplTag>(indexBuffer));
}
void RenderCommandEncoder::bindVertexBuffer(uint32_t binding, uint64_t offset, Buffer &vertexBuffer) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    encoder->bindVertexBuffer(binding, offset, *(VK::Buffer *)Access::get<BufferImplTag>(vertexBuffer));
}

void RenderCommandEncoder::bindVertexBuffer(uint32_t binding, uint32_t bindingCount, const uint64_t *offset, Buffer *const *vertexBuffer) {
    VK::RenderCommandEncoder *encoder = (VK::RenderCommandEncoder *)impl;
    VK::Buffer **buffers = (VK::Buffer **)alloca(bindingCount * sizeof(VkBuffer));
    for (uint32_t i = 0; i < bindingCount; ++i) {
        buffers[i] = (VK::Buffer *)Access::get<BufferImplTag>(*(vertexBuffer[i]));
    }
    encoder->bindVertexBuffer(binding, bindingCount, offset, buffers);
}

}