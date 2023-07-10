//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_STATICMESHNODE_HPP
#define OJOIE_STATICMESHNODE_HPP

#include <ojoie/Render/RenderQueue.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Node/Node3D.hpp>
#include <string>
#include <vector>

namespace AN {


struct StaticMeshNodeTextureInfo {
    const char *name;
//    TextureType type;
};

class StaticMeshNode : public Node3D {
    typedef StaticMeshNode Self;
    typedef Node3D Super;


    Math::vec4 _color;
    bool didSetColor{};

//    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    struct __StaticMeshNodeTextureInfo {
        std::string name;
//        TextureType type;
    };

    std::vector<__StaticMeshNodeTextureInfo> textureInfos;

    virtual bool init() override { return false; }
public:

//    struct StaticMeshNodeSceneProxy : Super::SceneProxyType {
//
//        /// \RenderActor
//        Mesh mesh;
////        std::vector<RC::Texture> textures;
//
//        Math::mat4 preModel{ 1.f };
//
//        explicit StaticMeshNodeSceneProxy(StaticMeshNode &node) : Node3DSceneProxy(node) {
//            mesh.setColor(node._color);
//        }
//
//        virtual bool createRenderResources() override;
//        virtual void destroyRenderResources() override;
//
//        virtual void render(const RenderContext &context) override;
//    };

//    typedef StaticMeshNodeSceneProxy SceneProxyType;

    StaticMeshNode();

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

//    virtual bool init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t indicesNum);

//    virtual bool init(Vertex *vertices, uint64_t verticesNum,
//                      uint32_t *indices, uint64_t indicesNum,
//                      StaticMeshNodeTextureInfo *textures, uint64_t textureNum);


//    virtual RC::SceneProxy *createSceneProxy() override {
//        return new StaticMeshNodeSceneProxy(*this);
//    }

    virtual void updateSceneProxy() override {
        Super::updateSceneProxy();
        if (didSetColor) {
            sceneProxy->retain();
            GetRenderQueue().enqueue([color = _color, sceneProxy = sceneProxy] {
//                auto *proxy = (StaticMeshNodeSceneProxy *)(sceneProxy);
//                proxy->mesh.setColor(color);
//                proxy->release();
            });

            didSetColor = false;
        }
    }

    /// \brief set color if no texture provides
    void setColor(Math::vec4 color) {
        _color = color;
        didSetColor = true;
    }

    virtual ~StaticMeshNode() override;

};



}
#endif//OJOIE_STATICMESHNODE_HPP
