//
// Created by aojoie on 6/24/2023.
//

#pragma once

#include "EditorPanel.hpp"

namespace AN::Editor {

class InspectorPanel : public Panel {

    Vector3f position;
    Vector3f rotation;
    Vector3f scale;
    int revertButtonId;

    bool revertButton();

public:
    virtual void onGUI() override;
};


}