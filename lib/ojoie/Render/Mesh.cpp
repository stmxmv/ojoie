//
// Created by Aleudillonam on 8/7/2022.
//
#include "Render/Mesh.hpp"
#include "Core/Dispatch.hpp"
#include <glad/glad.h>

namespace AN {




Mesh::~Mesh() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    
    if (hasTextures) {
        _textures.~vector<TextureInfo>();
    } else {
        _color.~vec();
    }
}

bool Mesh::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum) {
    _indicesNum = indicesNum;
    _color = DefaultColor();
    hasTextures = false;
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (int)(verticesNum * sizeof(Vertex)), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int)(indicesNum * sizeof(uint32_t)), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoord)));
    glEnableVertexAttribArray(2);

    return vao != 0 && vbo != 0 && ebo != 0;
}

bool Mesh::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum, TextureInfo *textures, uint64_t textureNum) {
    if (Self::init(vertices, verticesNum, indices, indicesNum)) {
        hasTextures = true;
        new ((void *)&_textures) std::vector<TextureInfo>(textures, textures + textureNum);
        return true;
    }
    return false;
}

void Mesh::setColor(Math::vec4 color) {
    _color = color;
}

void Mesh::render(const RenderPipeline &pipeline) {

    if (!hasTextures) {
        pipeline.setVec4("color", _color);
    } else {
        
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (unsigned int i = 0; i < (unsigned int)_textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            unsigned int number;
            const char * name;
            switch (_textures[i].type) {
                case TextureType::diffuse:
                    name = "texture_diffuse";
                    number = diffuseNr++;
                    break;
                case TextureType::specular:
                    name = "texture_specular";
                    number = specularNr++;
                    break;
                default:
                    continue;
            }

            pipeline.setInt(std::format("material.{}{}", name, number).c_str(), i);

            glBindTexture(GL_TEXTURE_2D, _textures[i].id);
        }

    }
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, _indicesNum, GL_UNSIGNED_INT, nullptr);

}

}