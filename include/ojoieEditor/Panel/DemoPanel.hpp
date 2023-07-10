//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_DEMOPANEL_HPP
#define OJOIE_DEMOPANEL_HPP

#include <ojoieEditor/Panel/EditorPanel.hpp>

namespace AN::Editor {

class DemoPanel : public Panel {
    typedef DemoPanel Self;
    typedef Panel Super;
    bool show;
public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init() override {
        if (Super::init()) {
            show = true;
            return true;
        }
        return false;
    }

    virtual void draw(const RenderContext &context) override {
        Super::draw(context);
        if (show) {
            ImGui::ShowDemoWindow(&show);
        }
    }
};

}

#endif//OJOIE_DEMOPANEL_HPP
