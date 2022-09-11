//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_IMGUINODE_HPP
#define OJOIE_IMGUINODE_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/UI/Imgui.hpp>

namespace AN {


class ImguiNode : public Node {
    typedef ImguiNode Self;
public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    ImguiNode() : Node(true) {}

    virtual bool init() override;

    virtual void render(const RenderContext &context) override;

    void newFrame(const RenderContext &context);
    void endFrame(const RenderContext &context);
};


class TestImguiNode : public ImguiNode {

    typedef TestImguiNode Self;
public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


    virtual void render(const RenderContext &context) override;
};


}


#endif//OJOIE_IMGUINODE_HPP
