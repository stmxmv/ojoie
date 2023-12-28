//
// Created by aojoie on 6/24/2023.
//

#include "Panels/HierarchyPanel.hpp"

#include <imgui_stdlib.h>

#include "ojoie/Editor/Selection.hpp"
#include <ojoie/Core/Actor.hpp>
#include <ojoie/Geometry/Cube.hpp>
#include <ojoie/Geometry/Sphere.hpp>
#include <ojoie/Geometry/Plane.hpp>
#include <ojoie/Render/Mesh/MeshRenderer.hpp>
#include <ojoie/Misc/ResourceManager.hpp>

namespace AN::Editor {

static std::vector<Actor *> rootActors;

void HierarchyPanel::AddRootActor(Actor *actor)
{
    rootActors.push_back(actor);
}

HierarchyPanel::HierarchyPanel() : createChildActor() {
    std::vector<Actor *> actors = Object::FindObjectsOfType<Actor>();

    /// keep only root actors
    for (auto it = actors.begin(); it != actors.end();) {
        Transform *transform = (*it)->getTransform();
        if (transform->getParent() || (*it)->getName() == "EditorManager") {
            it = actors.erase(it);
        } else {
            ++it;
        }
    }

    rootActors = std::move(actors);
}

void HierarchyPanel::drawTreeNode(int idx, Actor *root) {
    if (root) {
        Transform         *transform = root->getTransform();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
        if (!transform->getChildren().empty()) {
            flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        } else {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (root == Selection::GetActiveActor()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushID(root);

        bool opened = ImGui::TreeNodeEx(root, flags, "%s", root->getName().c_str());

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("Hierarchy_Actor", &root, sizeof root);
            // Display preview (could be anything, e.g. when dragging an image we could decide to display
            // the filename and a small preview of the image, etc.)
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Hierarchy_Actor")) {
                IM_ASSERT(payload->DataSize == sizeof root);
                Actor *actor = *(Actor **)payload->Data;
                actor->getTransform()->setParent(root->getTransform(), true);

                auto iter = std::find(rootActors.begin(), rootActors.end(), actor);
                if (iter != rootActors.end()) {
                    rootActors.erase(iter);
                }
            }
            ImGui::EndDragDropTarget();
        }


        if (ImGui::IsItemHovered() && root == Selection::GetActiveActor()) {
            selectionHover = true;
        }

        if (ImGui::IsItemClicked()) {
            Selection::SetActiveObject(root);
            clearSelection = false;
        }

        ImGui::PopID();

        if (opened) {
            auto &&children = transform->getChildren();
            for (int i = 0; i < children.size(); ++i) {
                drawTreeNode(i, children[i]->getActorPtr());
            }
            ImGui::TreePop();
        }

    }
}

void HierarchyPanel::onGUI() {

    clearSelection = true;
    selectionHover = false;

    ImGui::Begin("Hierarchy", getOpenPtr());

    for (int i = 0; i < rootActors.size(); ++i) {
        drawTreeNode(i, rootActors[i]);
    }

    if (!rootActors.empty()) {
        /// last dummy node
        {
//            ImGui::TreeNodeEx(rootActors.back() + 1, ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf, "");
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImGui::Dummy({ size.x, 20.f });
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Hierarchy_Actor")) {
                    IM_ASSERT(payload->DataSize == sizeof(void *));
                    Actor *actor = *(Actor **)payload->Data;
                    actor->getTransform()->setParent(nullptr, true);

                    auto iter = std::find(rootActors.begin(), rootActors.end(), actor);
                    if (iter == rootActors.end()) {
                        rootActors.push_back(actor);
                    }
                }
                ImGui::EndDragDropTarget();
            }
//            ImGui::TreePop();
        }
    }


    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered()) {
        ImGui::OpenPopup("HIERARCHY_CONTEXT");
        if (selectionHover) {
            createChildActor = true;
        }
    }

    /// context popup
    bool rename = false;
    if (ImGui::BeginPopup("HIERARCHY_CONTEXT")) {

        if (ImGui::MenuItem("Rename", nullptr, false, Selection::GetActiveActor() != nullptr)) {
            rename = true;
        }

        if (ImGui::MenuItem("Delete", nullptr, false, Selection::GetActiveActor() != nullptr)) {
            bool isRoot = Selection::GetActiveActor()->getTransform()->getParent() == nullptr;
            if (isRoot) {
                std::erase(rootActors, Selection::GetActiveActor());
            }
            DestroyActor(Selection::GetActiveActor());
            Selection::SetActiveObject(nullptr);
        }

        bool rootActive = Selection::GetActiveActor() != nullptr;
        if (ImGui::MenuItem("Move Up", nullptr, false, rootActive)) {
            Actor *actor = Selection::GetActiveActor();
            Transform *transform = actor->getTransform();
            if (transform->getParent() == nullptr) {
                auto iter = std::find(rootActors.begin(), rootActors.end(), actor);
                if (iter != rootActors.end() && iter != rootActors.begin()) {
                    std::iter_swap(iter, iter - 1);
                }
            } else {
                Transform *parent = transform->getParent();
                parent->moveChildUp(transform);
            }
        }

        if (ImGui::MenuItem("Move Down", nullptr, false, rootActive)) {
            Actor *actor = Selection::GetActiveActor();
            Transform *transform = actor->getTransform();
            if (transform->getParent() == nullptr) {
                auto iter = std::find(rootActors.begin(), rootActors.end(), actor);
                if (iter != rootActors.end() && iter != rootActors.end() - 1) {
                    std::iter_swap(iter, iter + 1);
                }
            } else {
                Transform *parent = transform->getParent();
                parent->moveChildDown(transform);
            }
        }

        if (ImGui::MenuItem("Create Empty")) {
            ImGui::CloseCurrentPopup();
            Actor *actor = NewObject<Actor>();
            actor->init();

            if (createChildActor) {
                actor->getTransform()->setParent(Selection::GetActiveActor()->getTransform(), false);
            } else {
                rootActors.push_back(actor);
            }
        }

        if (ImGui::BeginMenu("3D Object")) {
            if (ImGui::MenuItem("Cube")) {
                Actor *actor = NewObject<Actor>();
                actor->init("Cube");
                Mesh *cubeMesh = GetCubeMesh();

                MeshRenderer *meshRenderer = actor->addComponent<MeshRenderer>();
                meshRenderer->setMesh(cubeMesh);

                meshRenderer->setMaterial(0,  (Material *)GetResourceManager().getResource(Material::GetClassNameStatic(), "Default"));

                if (createChildActor) {
                    actor->getTransform()->setParent(Selection::GetActiveActor()->getTransform(), false);
                } else {
                    rootActors.push_back(actor);
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Sphere")) {
                Actor *actor = NewObject<Actor>();
                actor->init("Sphere");

                Mesh *cubeMesh = GetSphereMesh();

                MeshRenderer *meshRenderer = actor->addComponent<MeshRenderer>();
                meshRenderer->setMesh(cubeMesh);

                meshRenderer->setMaterial(0,  (Material *)GetResourceManager().getResource(Material::GetClassNameStatic(), "Default"));

                if (createChildActor) {
                    actor->getTransform()->setParent(Selection::GetActiveActor()->getTransform(), false);
                } else {
                    rootActors.push_back(actor);
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Plane")) {
                Actor *actor = NewObject<Actor>();
                actor->init("Plane");

                Mesh *cubeMesh = GetPlaneMesh();

                MeshRenderer *meshRenderer = actor->addComponent<MeshRenderer>();
                meshRenderer->setMesh(cubeMesh);

                meshRenderer->setMaterial(0,  (Material *)GetResourceManager().getResource(Material::GetClassNameStatic(), "Default"));

                if (createChildActor) {
                    actor->getTransform()->setParent(Selection::GetActiveActor()->getTransform(), true);
                } else {
                    rootActors.push_back(actor);
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    } else {
        createChildActor = false;
    }

    if (rename) {
        ImGui::OpenPopup("Rename##HIERARCHY_RENAME");
        inputTextBuffer = Selection::GetActiveActor()->getName().c_str();
    }

    if (ImGui::BeginPopupModal("Rename##HIERARCHY_RENAME", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Rename Actor:");
        bool enter = ImGui::InputText("##HIERARCHY_RENAMEinput", &inputTextBuffer, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("OK", ImVec2(240, 0)) || enter) {
            Selection::GetActiveActor()->setName(Name{ inputTextBuffer });
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(240, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (clearSelection && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow)) {
        if (Selection::GetActiveObject()) {
            Selection::SetActiveObject(nullptr);
        }
    }

    ImGui::End();
}

}