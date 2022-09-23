//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_STATICMESHNODE_HPP
#define OJOIE_STATICMESHNODE_HPP

#include <ojoie/Render/Mesh.hpp>
#include <ojoie/Node/Node3D.hpp>
#include <vector>

namespace AN {


class StaticMeshNode : public Node3D {
    typedef StaticMeshNode Self;
    typedef Node3D Super;


    struct Impl;
    Impl *impl;

    Math::mat4 preModel{ 1.f };

    virtual bool init() override { return false; }
public:
    StaticMeshNode();

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum);

    virtual bool init(Vertex *vertices, uint64_t verticesNum,
                      uint32_t *indices, uint64_t indicesNum,
                      TextureInfo *textures, uint64_t textureNum);

    /// \brief set color if no texture provides
    void setColor(Math::vec4 color);

    /// \brief copy a MeshNode which has the same underlying buffer
    std::shared_ptr<StaticMeshNode> copy();

    virtual ~StaticMeshNode() override;

    virtual void render(const RenderContext &context) override;
};



}
#endif//OJOIE_STATICMESHNODE_HPP
