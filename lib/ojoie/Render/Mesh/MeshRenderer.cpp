//
// Created by aojoie on 4/22/2023.
//

#include "Render/Mesh/MeshRenderer.hpp"
#include "Render/RenderContext.hpp"
#include "Render/CommandBuffer.hpp"

#include "Components/TransformComponent.hpp"
#include "Core/Actor.hpp"

#ifdef OJOIE_WITH_EDITOR
#include <ojoie/IMGUI/IMGUI.hpp>
#endif//OJOIE_WITH_EDITOR

namespace AN {

IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(MeshRenderer);
LOAD_AN_CLASS(MeshRenderer);

MeshRenderer::~MeshRenderer() {}

MeshRenderer::MeshRenderer(ObjectCreationMode mode) : Super(mode) {}

void MeshRenderer::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage,
                                              [](void *receiver, Message &message) {
                                                  MeshRenderer *meshRenderer = (MeshRenderer *) receiver;
                                                  meshRenderer->sendMessageSuper(message);
                                                  if (message.getData<Component *>() == meshRenderer) {
                                                      /// only do when the added component is self
                                                      meshRenderer->onAddMeshRenderer();
                                                  }
                                              });
}

void MeshRenderer::onAddMeshRenderer() {
    transform = getTransform();
}

void MeshRenderer::setMesh(Mesh *mesh) {
    _mesh = mesh;
}

void MeshRenderer::update(UInt32 frameIndex) {
    TransformComponent *transform = getTransform();
    if (transform) {
        transformData[frameIndex].objectToWorld = transform->getLocalToWorldMatrix();
        transformData[frameIndex].worldToObject = transform->getWorldToLocalMatrix();
    }
}

void MeshRenderer::render(RenderContext &renderContext, const char *pass) {
    if (_mesh == nullptr || transform == nullptr) return;

    for (int i = 0; i < _mesh->getSubMeshCount(); ++i) {
        SubMesh &subMesh = _mesh->getSubMesh(i);

        if (_materials.size() >= i + 1) {
            if (_materials[i] == nullptr) continue;
            Material &mat = *_materials[i];
            if (mat.hasPass(pass)) {
                /// setting per draw builtin properties
                mat.setVector("an_WorldTransformParams", { 0, 0, 0, 1.f });
                mat.setMatrix("an_ObjectToWorld", transformData[renderContext.frameIndex].objectToWorld);
                mat.setMatrix("an_WorldToObject", transformData[renderContext.frameIndex].worldToObject);

                mat.applyMaterial(renderContext.commandBuffer, pass);

//                if (strcmp(pass, "ShadowCaster") == 0) {
//                    renderContext.commandBuffer->setDepthBias(1.f, 2.5f);
//                }

                _mesh->getVertexBuffer().drawIndexed(renderContext.commandBuffer,
                                                     subMesh.indexCount,
                                                     subMesh.indexOffset,
                                                     0);
            }
        }
    }
}

#ifdef OJOIE_WITH_EDITOR
template<typename T, typename GetText, typename Add, typename Remove, typename DragAndDrop>
bool ArrayEdit(const char *label, const T *array, int size, GetText &&getText, Add &&add, Remove &&remove, DragAndDrop &&dragAndDrop) {
    ImGui::PushID(label);
    ImGui::Text(label);
    for (int i = 0; i < size; ++i) {
        ItemLabel(std::format("Element {}", i), kItemLabelLeft);
        std::string name = getText(array[i]);
        ImGui::InputText(std::format("##Element{}", i).c_str(), name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
        dragAndDrop(i);
    }
    ImVec2 textSize = ImGui::CalcTextSize("+");
    ImGuiStyle& style = ImGui::GetStyle();

    bool res = false;
    AlignForWidth(100.f, 1.f);
    if (ImGui::Button("+", { 40.f, 40.f })) {
        add();
        res = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("-", { 40.f, 40.f })) {
        remove();
        res = true;
    }
    ImGui::PopID();
    return res;
}

template<typename Container, typename GetText, typename Add, typename Remove, typename DragAndDrop>
bool ArrayEdit(const char *label, Container &container, GetText &&getText, Add &&add, Remove &&remove, DragAndDrop &&dragAndDrop) {
    return ArrayEdit(label, std::data(container), std::size(container), std::forward<GetText>(getText), std::forward<Add>(add), std::forward<Remove>(remove), std::forward<DragAndDrop>(dragAndDrop));
}
#endif//OJOIE_WITH_EDITOR

void MeshRenderer::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    ItemLabel("Mesh", kItemLabelLeft);
    std::string meshName = "None";
    if (_mesh) {
        meshName = _mesh->getName().string_view();
    }

    ImGui::InputText("##mesh", meshName.data(), meshName.size(), ImGuiInputTextFlags_ReadOnly);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_MESH")) {
            IM_ASSERT(payload->DataSize == sizeof(void *));
            Mesh *mesh = *(Mesh **)payload->Data;
            setMesh(mesh);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Dummy({ 0.f, 10.f });

    auto getMaterialName = [](const Material *mat) {
        if (mat == nullptr) { return "None"; }
        return mat->getName().c_str();
    };

    auto add = [&]() {
        _materials.emplace_back();
    };

    auto remove = [&]() {
        _materials.pop_back();
    };

    auto dragAnddrop = [&](int idx) {
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_MATERIAL")) {
                IM_ASSERT(payload->DataSize == sizeof(void *));
                Material *material = *(Material **)payload->Data;
                setMaterial(idx, material);
            }
            ImGui::EndDragDropTarget();
        }
    };;

    ArrayEdit("Materials", _materials, getMaterialName, add, remove, dragAnddrop);

    ImGui::Dummy({ 0.f, 10.f });
    ImGui::Separator();
    ImGui::Dummy({ 0.f, 10.f });

    for (auto &mat : _materials) {
        if (mat == nullptr) continue;
        ImGui::PushID(&mat);
        ImGui::Text("%s", mat->getName().c_str());
        mat->onInspectorGUI();
        ImGui::Separator();
        ImGui::PopID();
    }
#endif//OJOIE_WITH_EDITOR
}

}