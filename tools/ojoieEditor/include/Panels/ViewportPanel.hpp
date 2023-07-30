//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_VIEWPORTPANEL_HPP
#define OJOIE_VIEWPORTPANEL_HPP

#include "EditorPanel.hpp"
#include <ojoie/Render/Texture2D.hpp>
#include <ojoie/Render/RenderTarget.hpp>
#include <ojoie/Object/ObjectPtr.hpp>
#include <ImGuizmo.h>


namespace AN::Editor {

class ViewportPanel : public Panel {

    ObjectPtr<RenderTarget> sceneTarget;

    bool dragAndDropUpdating = false;
    bool bMouseHover;
    bool bFocus;

    ImGuizmo::OPERATION gizmoType = ImGuizmo::TRANSLATE;

    class SceneBehavior *sceneBehavior;

public:

    ViewportPanel();

    virtual void onGUI() override;

    bool isMouseHover() const { return bMouseHover; }
    bool isFocus() const { return bFocus; }
};

}

#endif//OJOIE_VIEWPORTPANEL_HPP
