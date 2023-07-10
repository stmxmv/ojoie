//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_EDITORPANEL_HPP
#define OJOIE_EDITORPANEL_HPP

#include "ojoie/UI/Imgui.hpp"

namespace AN::Editor {


class Panel : public std::enable_shared_from_this<Panel> {

public:

    virtual bool init() { return true; };
    virtual void update(float deltaTime) {}
    virtual void draw(const AN::RenderContext &context, const char* title, bool* p_open = nullptr) {}

};

}

#endif//OJOIE_EDITORPANEL_HPP
