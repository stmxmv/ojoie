//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_STATICMODELNODE_HPP
#define OJOIE_STATICMODELNODE_HPP

#include <ojoie/Node/Node3D.hpp>
#include <ojoie/Node/StaticMeshNode.hpp>
#include <ojoie/Render/Model.hpp>

namespace AN {



class StaticModelNode : public Node3D {
    typedef StaticModelNode Self;
    typedef Node3D Super;

    virtual bool init() override { return false; }

    std::string modelPath;
public:

    struct StaticModelNodeSceneProxy : Super::SceneProxyType {
        Model model;

        Math::mat4 preModel{ 1.f };

        explicit StaticModelNodeSceneProxy(StaticModelNode &node) : Node3DSceneProxy(node) {}


        virtual bool createRenderResources() override;
        virtual void destroyRenderResources() override;
        virtual void render(const RenderContext &context) override;
    };

    typedef StaticModelNodeSceneProxy SceneProxyType;

    StaticModelNode();

    ~StaticModelNode();

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual RC::SceneProxy *createSceneProxy() override {
        return new StaticModelNodeSceneProxy(*this);
    }

    virtual bool init(const char *modelPath);

};


}


#endif//OJOIE_STATICMODELNODE_HPP
