//
// Created by aojoie on 4/22/2023.
//

#include "Render/Mesh/MeshRenderer.hpp"
#include "Render/RenderContext.hpp"
#include "Render/CommandBuffer.hpp"

#include "Components/TransformComponent.hpp"
#include "Core/Actor.hpp"

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
            Material &mat = *_materials[i];

            /// setting per draw builtin properties
            mat.setVector("an_WorldTransformParams", { 0, 0, 0, 1.f });
            mat.setMatrix("an_ObjectToWorld", transformData[renderContext.frameIndex].objectToWorld);
            mat.setMatrix("an_WorldToObject", transformData[renderContext.frameIndex].worldToObject);

            mat.applyMaterial(renderContext.commandBuffer, pass);

            _mesh->getVertexBuffer().drawIndexed(renderContext.commandBuffer,
                                                 subMesh.indexCount,
                                                 subMesh.indexOffset,
                                                 0);
        }
    }
}

}