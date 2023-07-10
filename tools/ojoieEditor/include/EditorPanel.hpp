//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_EDITORPANEL_HPP
#define OJOIE_EDITORPANEL_HPP

#include <ojoie/IMGUI/IMGUI.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Template/RC.hpp>

namespace AN::Editor {


class Panel : public RefCounted<Panel> {
    bool open;
    bool closeable;
public:

    Panel() : open(true), closeable(true) {}

    virtual ~Panel() = default;
    virtual void onGUI() {}

    bool isOpened() const { return open; }
    void setOpen(bool isOpened) { open = isOpened; }
    bool isCloseable() const { return closeable; }
    void setCloseable(bool able) { closeable = able; }

    bool *getOpenPtr() { return &open; }
};

}

#endif//OJOIE_EDITORPANEL_HPP
