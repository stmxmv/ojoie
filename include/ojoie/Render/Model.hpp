//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_MODEL_HPP
#define OJOIE_MODEL_HPP

#include <ojoie/Render/Mesh.hpp>

namespace AN {


struct ModelTexture {
    uint64_t id; /// this could be a pointer
    TextureType type;
    std::string path;
};


struct SubMesh {
    std::vector<TextureInfo> textures;
    int indexCount;
    int indexOffset;
};

class Model {
    std::vector<SubMesh> meshes;
    std::vector<ModelTexture> loadedTextures;

    struct Impl;
    Impl *impl;

public:

    Model();

    ~Model();

    bool init(const char *modelPath);


    void render(const RenderPipeline &pipeline);

};


}

#endif//OJOIE_MODEL_HPP
