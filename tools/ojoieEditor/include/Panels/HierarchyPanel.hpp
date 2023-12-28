//
// Created by aojoie on 6/24/2023.
//

#pragma once

#include "EditorPanel.hpp"

namespace AN::Editor {

class HierarchyPanel : public Panel {
    std::string inputTextBuffer;

    bool clearSelection;
    bool selectionHover;
    bool createChildActor;
    void drawTreeNode(int idx, Actor *root);

public:

    static void AddRootActor(Actor *actor);

    HierarchyPanel();

    virtual void onGUI() override;
};

}
