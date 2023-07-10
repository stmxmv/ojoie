//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_VIEWPORTPANEL_HPP
#define OJOIE_VIEWPORTPANEL_HPP

#include "EditorPanel.hpp"
#include <ojoie/Render/Texture2D.hpp>
#include <ojoie/Render/RenderTarget.hpp>
#include <ojoie/Object/ObjectPtr.hpp>

namespace AN::Editor {

class ViewportPanel : public Panel {

    ObjectPtr<RenderTarget> sceneTarget;

    bool dragAndDropUpdating = false;

public:

    ViewportPanel();

    virtual void onGUI() override;
};

}

#endif//OJOIE_VIEWPORTPANEL_HPP
