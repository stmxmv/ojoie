//
// Created by aojoie on 6/24/2023.
//

#include "Panels/InspectorPanel.hpp"
#include "Selection.hpp"

namespace AN::Editor {

bool InspectorPanel::revertButton() {
    ImGui::PushID(std::format("{}_{}", (uint64_t)this, revertButtonId++).c_str());
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    bool ret = ImGui::Button("Revert");
    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return ret;
}

void InspectorPanel::onGUI() {
    ImGui::Begin("Inspector", getOpenPtr());

    ImGui::PushID(this);
    static const char *position_str = "P";
    ImGui::Text("%s", position_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(position_str).x) * 0.25f );

    Actor *actor = Selection::GetActiveActor();
    TransformComponent *transform = nullptr;
    if (actor) {
        transform = actor->getTransform();
        position = transform->getLocalPosition();
        rotation = transform->getEulerAngles();
        scale = transform->getLocalScale();
    }

    if (ImGui::DragFloat("X##position", &position.x, 0.1f)) {
        if (transform) {
            transform->setLocalPosition(position);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y##position", &position.y, 0.1f)) {
        if (transform) {
            transform->setLocalPosition(position);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Z##position", &position.z, 0.1f)) {
        if (transform) {
            transform->setLocalPosition(position);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (revertButton()) {
        position = {};
    }
    ImGui::PopItemWidth();
    //            ImGui::EndChild();


    static const char *rotation_str = "R";
    ImGui::Text("%s", rotation_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(rotation_str).x) * 0.25f );
    if (ImGui::DragFloat("P", &rotation.x, 1.f, -180.f, 180.f)) {
        if (transform) {
            Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
            transform->setLocalRotation(Math::toQuat(mat));
        }
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y", &rotation.y, 1.f, -180.f, 180.f)) {
        if (transform) {
            Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
            transform->setLocalRotation(Math::toQuat(mat));
        }
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("R", &rotation.z, 1.f, -180.f, 180.f)) {
        if (transform) {
            Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
            transform->setLocalRotation(Math::toQuat(mat));
        }
    }
    ImGui::SameLine();
    if (revertButton()) {
    }
    ImGui::PopItemWidth();



    static const char *scale_str = "S";
    ImGui::Text("%s", scale_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(scale_str).x) * 0.25f );
    if (ImGui::DragFloat("X##scale", &scale.x, 0.1f, 0.f)) {
        if (transform) {
            transform->setLocalScale(scale);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y##scale", &scale.y, 0.1f, 0.f)) {
        if (transform) {
            transform->setLocalScale(scale);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Z##scale", &scale.z, 0.1f, 0.f)) {
        if (transform) {
            transform->setLocalScale(scale);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (revertButton()) {
        scale = { 1.f, 1.f, 1.f };
    }

    ImGui::PopItemWidth();

    ImGui::PopID();

    ImGui::End();
}

}