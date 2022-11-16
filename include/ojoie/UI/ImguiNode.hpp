//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_IMGUINODE_HPP
#define OJOIE_IMGUINODE_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/UI/Imgui.hpp>

namespace AN {


class ImguiNode : public Node {
    typedef Node Super;
    typedef ImguiNode Self;
public:

    struct ImguiNodeSceneProxy : Super::SceneProxyType {

        ImguiNodeSceneProxy(ImguiNode &node) : NodeSceneProxy(node) {}

        void newFrame(const RenderContext &context);
        void endFrame(const RenderContext &context);

        virtual void postRender(const RenderContext &context) override;
    };

    typedef ImguiNodeSceneProxy SceneProxyType;

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    ImguiNode() {
        _postRender = true;
    }

    virtual bool init() override;

    virtual RC::SceneProxy *createSceneProxy() override {
        return new ImguiNodeSceneProxy(*this);
    }

    static UI::Imgui &GetImGuiInstance();
};


class TestImguiNode : public ImguiNode {
    typedef ImguiNode Super;
    typedef TestImguiNode Self;
public:

    struct TestImguiNodeSceneProxy : Super::SceneProxyType {

        explicit TestImguiNodeSceneProxy(ImguiNode &node) : Super::SceneProxyType(node) {}

        virtual void postRender(const RenderContext &context) override;
    };

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual RC::SceneProxy *createSceneProxy() override {
        return new TestImguiNodeSceneProxy(*this);
    }
};


}


#endif//OJOIE_IMGUINODE_HPP
