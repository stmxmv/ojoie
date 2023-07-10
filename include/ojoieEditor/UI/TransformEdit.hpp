//
// Created by aojoie on 10/23/2022.
//

#ifndef OJOIE_TRANSFORMEDIT_HPP
#define OJOIE_TRANSFORMEDIT_HPP

#include <ojoie/Node/Node3D.hpp>
#include <ojoie/UI/Imgui.hpp>

namespace AN::Editor {

class TransformEdit : public std::enable_shared_from_this<TransformEdit> {
    typedef TransformEdit Self;

    std::weak_ptr<AN::Node3D> _mesh;

    bool didSetPosition{};
    bool didFinishSetPosition{};
    AN::Math::vec3 position{};
    AN::Math::vec3 position_min{ -10.f, -10.f, -10.f };
    AN::Math::vec3 position_max{ 10.f, 10.f, 10.f };

    bool didSetRotation{};
    AN::Math::rotator rotation{};

    bool didSetScale{};
    bool didFinishSetScale{};
    AN::Math::vec3 scale{};
    AN::Math::vec3 scale_max{ 2.f, 2.f, 2.f };

    int revertButtonId;
    bool revertButton() {
        ImGui::PushID(std::format("{}_{}", (uint64_t)this, revertButtonId++).c_str());
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        bool ret = ImGui::Button("Revert");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        return ret;
    }

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    TransformEdit() = default;

    bool init(const std::shared_ptr<AN::Node3D> &mesh) {
        _mesh = mesh;
        return true;
    }

    bool init() {
        return init({});
    }

    void setNode(const std::shared_ptr<AN::Node3D> &node) {
        _mesh = node;
    }

    void update() {
        std::shared_ptr<AN::Node3D> mesh = _mesh.lock();
        if (mesh) {
            AN::Dispatch::async(
                    AN::Dispatch::Render,
                    [position = mesh->getPosition(), rotation = mesh->getRotation(), scale = mesh->getScale(), self = shared_from_this()] {
                        self->position = position;
                        self->rotation = rotation;
                        self->scale    = scale;
                    });
        }
    }

    void draw() {
        ImGui::PushID(this);
        revertButtonId = 0;
        static const char *position_str = "P";
        ImGui::Text("%s", position_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(position_str).x) * 0.25f );
        if (ImGui::DragFloat("X##position", &position.x, 0.1f, position_min.x, position_max.x)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##position", &position.y, 0.1f, position_min.y, position_max.y)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##position", &position.z, 0.1f, position_min.z, position_max.z)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetPosition = true;
            didFinishSetPosition = true;
            position = {};
        }
        ImGui::PopItemWidth();
        //            ImGui::EndChild();


        static const char *rotation_str = "R";
        ImGui::Text("%s", rotation_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(rotation_str).x) * 0.25f );
        if (ImGui::DragFloat("P", &rotation.pitch, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y", &rotation.yaw, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("R", &rotation.roll, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetRotation = true;
            rotation = {};
        }
        ImGui::PopItemWidth();



        static const char *scale_str = "S";
        ImGui::Text("%s", scale_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(scale_str).x) * 0.25f );
        if (ImGui::DragFloat("X##scale", &scale.x, 0.1f, 0.f, scale_max.x)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##scale", &scale.y, 0.1f, 0.f, scale_max.y)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##scale", &scale.z, 0.1f, 0.f, scale_max.z)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetScale = true;
            didFinishSetScale = true;
            scale = { 1.f, 1.f, 1.f };
        }

        ImGui::PopItemWidth();

        ImGui::PopID();


        if (didSetPosition) {
            didSetPosition = false;
            AN::Dispatch::async(AN::Dispatch::Game, [position = position, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setPosition(position);
                }
            });
        }

        if (didFinishSetPosition) {
            didFinishSetPosition = false;
            position_min = position - 10.f;
            position_max = position + 10.f;
        }

        if (didSetRotation) {
            didSetRotation = false;
            AN::Dispatch::async(AN::Dispatch::Game, [rotation = rotation, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setRotation(rotation);
                }
            });
        }

        if (didSetScale) {
            didSetScale = false;
            AN::Dispatch::async(AN::Dispatch::Game, [scale = scale, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setScale(scale);
                }
            });
        }

        if (didFinishSetScale) {
            didFinishSetScale = false;
            scale_max = scale * 2.f;
        }
    }


};


}

#endif//OJOIE_TRANSFORMEDIT_HPP
