//
// Created by aojoie on 6/24/2023.
//

#include "Panels/InspectorPanel.hpp"
#include "ojoie/Editor/Selection.hpp"

namespace AN::Editor {

static void AlignForWidth(float width, float alignment = 0.5f) {
    ImGuiStyle &style = ImGui::GetStyle();
    float       avail = ImGui::GetContentRegionMax().x;
    float       off   = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(off);
}


void InspectorPanel::onGUI() {
    ImGui::Begin("Inspector", getOpenPtr());

    if (Selection::GetActiveActor() == nullptr) {
        ImGui::End();
        return;
    }

    ImGui::PushID(this);

    Actor *actor      = Selection::GetActiveActor();
    auto  &components = actor->getComponents();

    Component *destroyComponent = nullptr;
    for (auto &pair : components) {
        Component *component = pair.second;
        ImGui::PushID(component);
        std::string componentName = component->getClassName();
        if (auto it = componentName.find("Component"); it != std::string::npos) {
            componentName.erase(it);
        }
        bool opened = ImGui::TreeNodeEx(componentName.c_str(),
                                        ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::SameLine();
        AlignForWidth(40.f, 1.f);
        if (ImGui::Button("...")) {
            ImGui::OpenPopup("Inspector_Component_Context");
        }

        if (ImGui::BeginPopup("Inspector_Component_Context")) {
            if (ImGui::MenuItem("Remove Component")) {
                /// transform cannot remove
                if (component->getClassID() != TransformComponent::GetClassIDStatic()) {
                    destroyComponent = component;
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (opened) {
            component->onInspectorGUI();
        }
        ImGui::PopID();
    }

    if (destroyComponent) {
        DestroyObject(destroyComponent);
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    const char *addComText = "Add Component";
    AlignForWidth(400.f, 0.5f);

    if (ImGui::Button(addComText, { 400.f, 0.f })) {
        ImGui::OpenPopup("Inspector_Add_Component");
    }

    if (ImGui::BeginPopup("Inspector_Add_Component")) {

        std::vector<Class *> componentClasses = Class::FindAllSubClasses<Component>();

        std::sort(componentClasses.begin(), componentClasses.end(), [](Class *a, Class *b) {
            return std::string_view{ a->getClassName() } < std::string_view{ b->getClassName() };
        });

        for (Class *cls : componentClasses) {
            if (cls->isAbstract()) continue;
            /// cannot add Transform
            if (strcmp(cls->getClassName(), TransformComponent::GetClassNameStatic()) == 0) {
                continue;
            }
            if (ImGui::MenuItem(cls->getClassName())) {
                actor->addComponentInternal(cls->getClassId());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();

    ImGui::End();
}

}// namespace AN::Editor