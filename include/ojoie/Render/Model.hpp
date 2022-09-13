//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_MODEL_HPP
#define OJOIE_MODEL_HPP

#include <ojoie/Render/Mesh.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/IndexBuffer.hpp>
#include <ojoie/Render/UniformBuffer.hpp>

namespace AN {


struct ModelTexture {
    RC::Texture texture; /// this could be a pointer
    TextureType type;
    std::string path;
};

struct ModelSubMeshTexture {
    uint64_t index;
    TextureType type;
};


struct SubMesh {
    std::vector<ModelSubMeshTexture> textures;
    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t vertexOffset;
};

class Model {
    std::vector<SubMesh> meshes;
    std::vector<ModelTexture> loadedTextures;

    struct Impl;
    Impl *impl;

    RC::VertexBuffer vertexBuffer;
    RC::IndexBuffer indexBuffer;

    RC::UniformBuffer lightUniformBuffer;

public:

    bool init(const char *modelPath);

    void deinit();

    void render();

};


}

#endif//OJOIE_MODEL_HPP
