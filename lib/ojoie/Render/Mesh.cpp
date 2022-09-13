//
// Created by Aleudillonam on 8/7/2022.
//
#include "Render/Mesh.hpp"
#include "Core/Dispatch.hpp"
#include "Render/Renderer.hpp"
#include <ojoie/Render/Sampler.hpp>
#include <glad/glad.h>

namespace AN {


static RC::Sampler sampler;

void Mesh::deinit() {
    vertexBuffer.deinit();
    indexBuffer.deinit();
    lightContext.deinit();
    
    if (hasTextures) {
        _textures.~vector<TextureInfo>();
    } else {
        _color.~vec();
    }
}

struct uniformStruct {
    alignas(16) Math::vec4 color;
    alignas(16) Math::vec3 lightPos;
    alignas(16) Math::vec3 lightColor;
};

struct uniformStructTextured {
    alignas(16) Math::vec3 lightPos;
    alignas(16) Math::vec3 lightColor;
};


bool Mesh::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum) {
    _indicesNum = indicesNum;
    _color = DefaultColor();
    hasTextures = false;

    const RenderContext &context = GetRenderer().getRenderContext();

    uint64_t verticesBytes = verticesNum * sizeof(Vertex);
    uint64_t indexBytes = indicesNum * sizeof(uint32_t);

    RC::BufferBlock stageBufferBlock = context.stageBufferPool.bufferBlock(verticesBytes + indexBytes);
    RC::BufferAllocation stageBufferAllocation = stageBufferBlock.allocate(verticesBytes + indexBytes);

    void *stageBufferData = stageBufferAllocation.map();
    memcpy(stageBufferData, vertices, verticesBytes);
    memcpy((char *)stageBufferData + verticesBytes, indices, indexBytes);

    stageBufferAllocation.getBuffer().flush();

    if (!vertexBuffer.init(verticesBytes)) {
        return false;
    }

    if (!indexBuffer.init(indexBytes)) {
        return false;
    }

    RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset(), vertexBuffer.getBuffer(), 0, verticesBytes);
    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset() + verticesBytes, indexBuffer.getBuffer(), 0, indexBytes);

    RC::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcStageFlag = RC::PipelineStageFlag::Transfer;
    bufferMemoryBarrier.srcAccessMask = RC::PipelineAccessFlag::TransferWrite;
    bufferMemoryBarrier.dstStageFlag = RC::PipelineStageFlag::VertexShader;
    bufferMemoryBarrier.dstAccessMask = RC::PipelineAccessFlag::ShaderRead;

    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = verticesBytes;

    blitCommandEncoder.bufferMemoryBarrier(vertexBuffer.getBuffer(), bufferMemoryBarrier);

    bufferMemoryBarrier.size = indexBytes;
    blitCommandEncoder.bufferMemoryBarrier(indexBuffer.getBuffer(), bufferMemoryBarrier);


    if (!lightContext.init(sizeof(uniformStruct))) {
        return false;
    }

    return true;
}

bool Mesh::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum, TextureInfo *textures, uint64_t textureNum) {
    _indicesNum = indicesNum;
    hasTextures = true;

    const RenderContext &context = GetRenderer().getRenderContext();

    uint64_t verticesBytes = verticesNum * sizeof(Vertex);
    uint64_t indexBytes = indicesNum * sizeof(uint32_t);

    RC::BufferBlock stageBufferBlock = context.stageBufferPool.bufferBlock(verticesBytes + indexBytes);
    RC::BufferAllocation stageBufferAllocation = stageBufferBlock.allocate(verticesBytes + indexBytes);

    void *stageBufferData = stageBufferAllocation.map();
    memcpy(stageBufferData, vertices, verticesBytes);
    memcpy((char *)stageBufferData + verticesBytes, indices, indexBytes);

    stageBufferAllocation.getBuffer().flush();

    if (!vertexBuffer.init(verticesBytes)) {
        return false;
    }

    if (!indexBuffer.init(indexBytes)) {
        return false;
    }

    RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset(), vertexBuffer.getBuffer(), 0, verticesBytes);
    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset() + verticesBytes, indexBuffer.getBuffer(), 0, indexBytes);

    RC::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcStageFlag = RC::PipelineStageFlag::Transfer;
    bufferMemoryBarrier.srcAccessMask = RC::PipelineAccessFlag::TransferWrite;
    bufferMemoryBarrier.dstStageFlag = RC::PipelineStageFlag::VertexShader;
    bufferMemoryBarrier.dstAccessMask = RC::PipelineAccessFlag::ShaderRead;

    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = verticesBytes;

    blitCommandEncoder.bufferMemoryBarrier(vertexBuffer.getBuffer(), bufferMemoryBarrier);

    bufferMemoryBarrier.size = indexBytes;
    blitCommandEncoder.bufferMemoryBarrier(indexBuffer.getBuffer(), bufferMemoryBarrier);


    new ((void *)&_textures) std::vector<TextureInfo>(textures, textures + textureNum);

    if (!lightContext.init(sizeof(uniformStructTextured))) {

        return false;
    }

    static bool samplerInited = false;
    if (!samplerInited) {
        samplerInited = true;
        RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();

        if (!sampler.init(samplerDescriptor)) {
            return false;
        }

        GetRenderQueue().registerCleanupTask([] {
            sampler.deinit();
        });
    }


    return true;
}

void Mesh::setColor(Math::vec4 color) {
    _color = color;
}


void Mesh::render(const struct AN::RenderContext &context, RC::RenderPipeline &pipeline) {
    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;
    if (!hasTextures) {

        uniformStruct *uniform = (uniformStruct *)(lightContext.content());
        uniform->color = _color;
        uniform->lightPos = { 1.2f, 10.0f, 2.0f };
        uniform->lightColor = { 0.980f, 0.976f, 0.902f };

        renderCommandEncoder.bindUniformBuffer(1, lightContext.getOffset(), lightContext.getSize(), lightContext.getBuffer());

    } else {

        renderCommandEncoder.bindSampler(2, sampler);

        uniformStructTextured *uniform = (uniformStructTextured *)lightContext.content();

        uniform->lightPos = { 1.2f, 10.0f, 2.0f };
        uniform->lightColor = { 0.980f, 0.976f, 0.902f };

        renderCommandEncoder.bindUniformBuffer(1, lightContext.getOffset(), lightContext.getSize(), lightContext.getBuffer());
        
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (unsigned int i = 0; i < (unsigned int)_textures.size(); i++) {
            unsigned int number;
            const char * name;
            switch (_textures[i].type) {
                case TextureType::diffuse:
                    renderCommandEncoder.bindTexture(3, *_textures[i].texture);
                    break;
                case TextureType::specular:
                    name = "texture_specular";
                    number = specularNr++;
                    break;
                default:
                    continue;
            }

        }

    }


    renderCommandEncoder.bindIndexBuffer(RC::IndexType::UInt32, indexBuffer.getBufferOffset(0), indexBuffer.getBuffer());
    renderCommandEncoder.bindVertexBuffer(0, vertexBuffer.getBufferOffset(0), vertexBuffer.getBuffer());

    renderCommandEncoder.drawIndexed(_indicesNum);

}

}