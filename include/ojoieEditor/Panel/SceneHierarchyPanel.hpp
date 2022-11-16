//
// Created by aojoie on 10/22/2022.
//

#ifndef OJOIE_SCENEHIERARCHYWINDOW_H
#define OJOIE_SCENEHIERARCHYWINDOW_H

#include <ojoie/Core/Node.hpp>
#include <ojoieEditor/Panel/EditorPanel.hpp>
#include <ojoieEditor/UI/TransformEdit.hpp>

namespace AN::Editor {

class SceneHierarchyPanel : public AN::Editor::Panel {
    typedef SceneHierarchyPanel Self;
    typedef AN::Editor::Panel Super;

    std::weak_ptr<AN::Node> _rootNode;

    std::weak_ptr<AN::Node> selectedNode;

    std::shared_ptr<TransformEdit> transformEdit;

    void drawTreeNode(const std::shared_ptr<AN::Node> &root) {
        if (root) {
            ImGuiTreeNodeFlags flags;
            if (root->children().count() != 0) {
                flags = ImGuiTreeNodeFlags_OpenOnArrow;
            } else {
                flags = ImGuiTreeNodeFlags_Leaf;
            }

            if (root.get() == selectedNode.lock().get()) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool opened = ImGui::TreeNodeEx(root.get(), flags, "%s", root->getName());

            if (ImGui::IsItemClicked()) {
                selectedNode = root->weak_from_this();
            }

            if (opened) {
                auto &&children = root->children();
                for (auto &&child : children) {
                    drawTreeNode(child);
                }

                ImGui::TreePop();
            }

        }
    }

    void drawNodeProperties(const std::shared_ptr<AN::Node> &root) {
        if (root) {
            // class name
#ifdef _MSC_VER
            const auto &node = *root;
            const char *className =  typeid(node).name() + 6;
#else
            const char *className = typeid(node).name();
#endif
            ImGui::Text("class name: %s", className);

            /// name, since we only change it on the render thread, it is fine
            char buffer[256];
            strcpy_s(buffer, sizeof buffer, root->getName());
            if (ImGui::InputText("Name", buffer, sizeof buffer)) {
                root->setName(buffer);
            }

            // transform edit if is node3d
            auto node3d = std::dynamic_pointer_cast<AN::Node3D>(root);
            if (node3d) {
                transformEdit->setNode(node3d);

                if (ImGui::TreeNodeEx(transformEdit.get(), ImGuiTreeNodeFlags_DefaultOpen, "Transform")) {
                    transformEdit->draw();
                    ImGui::TreePop();
                }

            }

            // camera node
            auto camera = std::dynamic_pointer_cast<AN::CameraNode>(root);
            if (camera) {
                float nearZ = camera->getNearZ();



            }

        }
    }

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    void setRootNode(const std::shared_ptr<AN::Node> &rootNode) {
        _rootNode = rootNode;
    }

    virtual bool init() override {
        if (Super::init()) {

            transformEdit = TransformEdit::Alloc();

            return transformEdit->init();
        }
        return false;
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);
        transformEdit->update();
    }

    virtual void draw(const RenderContext &context) override {
        Super::draw(context);

        ImGui::Begin("Scene Hierarchy");

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            selectedNode = {};
        }

        auto rootNode = _rootNode.lock();
        drawTreeNode(rootNode);

        ImGui::End();

        ImGui::Begin("Properties");

        drawNodeProperties(selectedNode.lock());
        ImGui::End();
    }
};



}

#endif//OJOIE_SCENEHIERARCHYWINDOW_H
