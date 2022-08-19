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

    if (!vertexBuffer.initStatic((void *)vertices, verticesNum * sizeof(Vertex))) {
        return false;
    }

    if (!indexBuffer.initStatic(indices, indicesNum * sizeof(uint32_t))) {
        return false;
    }

    if (!lightContext.init(sizeof(uniformStruct))) {
        return false;
    }

    return true;
}

bool Mesh::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum, TextureInfo *textures, uint64_t textureNum) {
    _indicesNum = indicesNum;
    hasTextures = true;

    if (!vertexBuffer.initStatic((void *)vertices, verticesNum * sizeof(Vertex))) {
        return false;
    }

    if (!indexBuffer.initStatic(indices, indicesNum * sizeof(uint32_t))) {
        return false;
    }


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

    if (!hasTextures) {

        uniformStruct *uniform = (uniformStruct *)(lightContext.content());
        uniform->color = _color;
        uniform->lightPos = { 1.2f, 10.0f, 2.0f };
        uniform->lightColor = { 0.980f, 0.976f, 0.902f };

        RC::BindUniformBuffer(1, lightContext);

    } else {

        RC::BindSampler(2, sampler);

        uniformStructTextured *uniform = (uniformStructTextured *)lightContext.content();

        uniform->lightPos = { 1.2f, 10.0f, 2.0f };
        uniform->lightColor = { 0.980f, 0.976f, 0.902f };

        RC::BindUniformBuffer(1, lightContext);
        
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (unsigned int i = 0; i < (unsigned int)_textures.size(); i++) {
            unsigned int number;
            const char * name;
            switch (_textures[i].type) {
                case TextureType::diffuse:
                    RC::BindTexture(3, *_textures[i].texture);
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


    indexBuffer.bind(RC::IndexType::UInt32);
    vertexBuffer.bind(0);

    RC::DrawIndexed(_indicesNum);

}

}