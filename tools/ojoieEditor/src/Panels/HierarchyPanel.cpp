//
// Created by aojoie on 6/24/2023.
//

#include "Panels/HierarchyPanel.hpp"
#include "Selection.hpp"
#include <ojoie/Core/Actor.hpp>

namespace AN::Editor {


void drawTreeNode(Actor *root) {
    if (root) {
        TransformComponent *transform = root->getTransform();
        ImGuiTreeNodeFlags flags;
        if (!transform->getChildren().empty()) {
            flags = ImGuiTreeNodeFlags_OpenOnArrow;
        } else {
            flags = ImGuiTreeNodeFlags_Leaf;
        }

        if (root == Selection::GetActiveActor()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool opened = ImGui::TreeNodeEx(root, flags, "%s", root->getName().c_str());

        if (ImGui::IsItemClicked()) {
            Selection::SetActiveObject(root);
        }

        if (opened) {
            auto &&children = transform->getChildren();
            for (auto &&child : children) {
                drawTreeNode(child->getActorPtr());
            }
            ImGui::TreePop();
        }

    }
}

void HierarchyPanel::onGUI() {
    ImGui::Begin("Hierarchy", getOpenPtr());

    std::vector<Actor *> actors = Object::FindObjectsOfType<Actor>();

    for (auto it = actors.begin(); it != actors.end();) {
        TransformComponent *transform = (*it)->getTransform();
        if (transform->getParent()) {
            it = actors.erase(it);
        } else {
            ++it;
        }
    }

    for (Actor *actor : actors) {
        drawTreeNode(actor);
    }

    ImGui::End();
}

}