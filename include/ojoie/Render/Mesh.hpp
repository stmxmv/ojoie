//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_MESH_HPP
#define OJOIE_MESH_HPP

#include <ojoie/Render/RenderPipeline.hpp>
#include <ojoie/Math/Math.hpp>
#include <vector>

namespace AN {

struct Vertex {
    Math::vec3 position;
    Math::vec3 normal;
    Math::vec2 texCoord;
};

enum class TextureType {
    diffuse,
    specular,
    normal,
    height
};

struct TextureInfo {
    uint64_t id; /// this could be a pointer
    TextureType type;
};

class Mesh : private NonCopyable {
    typedef Mesh Self;


    unsigned int vbo;
    unsigned int vao;
    unsigned int ebo;

    bool hasTextures;

    uint64_t _indicesNum;

    union {
        Math::vec4 _color;
        std::vector<TextureInfo> _textures;
    };


public:
    Mesh() {}

    ~Mesh();

    bool init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum);

    bool init(Vertex *vertices, uint64_t verticesNum,
                      uint32_t *indices, uint64_t indicesNum,
              TextureInfo *textures, uint64_t textureNum);

    Math::vec4 getColor() const {
        return _color;
    }

    /// \brief set color if no texture provides
    void setColor(Math::vec4 color);

    bool isTextured() const {
        return hasTextures;
    }

    void render(const RenderPipeline &context);


    static Math::vec4 DefaultColor() {
        return { 0.5f, 0.5f, 0.5f, 1.f };
    }
};



}

#endif//OJOIE_MESH_HPP
