//
// Created by Aleudillonam on 8/4/2023.
//

#include "Physics/MeshCollider.hpp"
#include "Render/Mesh/MeshRenderer.hpp"
#include "Core/Actor.hpp"

#include <algorithm>
#include <PxPhysicsAPI.h>

#ifdef OJOIE_WITH_EDITOR
#include "IMGUI/IMGUI.hpp"
#endif

namespace AN {

IMPLEMENT_AN_CLASS(MeshCollider)
LOAD_AN_CLASS(MeshCollider)

using namespace physx;
extern PxPhysics  *gPxPhysics;
extern PxScene    *gPxScene;
extern PxMaterial *gPxMaterial;
extern PxCooking  *gPxCooking;


MeshCollider::~MeshCollider() {
}

void MeshCollider::create(RigidBody *ignored) {
    if (m_Mesh == nullptr) {
        MeshRenderer *meshRenderer = getComponent<MeshRenderer>();
        if (meshRenderer) {
            m_Mesh = meshRenderer->getMesh();
        }

        if (m_Mesh == nullptr) return;
    }

    PxDefaultMemoryOutputStream writeBuffer;

    if (m_bConvex) {
        PxConvexMeshDesc convexDesc;
        convexDesc.setToDefault();

        convexDesc.points.count     = m_Mesh->getVertexCount();
        convexDesc.points.stride    = m_Mesh->getVertexBegin().getStride();
        convexDesc.points.data      = m_Mesh->getVertexBegin().getPointer();
        convexDesc.flags            = PxConvexFlag::eCOMPUTE_CONVEX;

        PxConvexMeshCookingResult::Enum result;
        bool status = gPxCooking->cookConvexMesh(convexDesc, writeBuffer, &result);
        if (!status || result == PxConvexMeshCookingResult::eFAILURE) {
            AN_LOG(Error, "Physx cook convex mesh fail");
            return;
        }

        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        PxConvexMesh *convexMesh = gPxPhysics->createConvexMesh(readBuffer);
        PxConvexMeshGeometry convexMeshGeometry(convexMesh);
        finishCreate(&convexMeshGeometry, ignored);
        convexMesh->release();

    } else {

        PxTriangleMeshDesc meshDesc;
        meshDesc.setToDefault();
        meshDesc.points.count  = m_Mesh->getVertexCount();
        meshDesc.points.stride = m_Mesh->getVertexBegin().getStride();
        meshDesc.points.data   = m_Mesh->getVertexBegin().getPointer();

        meshDesc.triangles.count  = m_Mesh->getIndicesCount() / 3;
        meshDesc.triangles.stride = 3 * sizeof(UInt16);
        meshDesc.triangles.data   = m_Mesh->getIndicesData();

        meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;

        PxTriangleMeshCookingResult::Enum result;
        bool status = gPxCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
        if (!status || result == PxTriangleMeshCookingResult::eFAILURE) {
            AN_LOG(Error, "Physx cook mesh fail");
            return;
        }

        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        PxTriangleMesh *triangleMesh = gPxPhysics->createTriangleMesh(readBuffer);
        PxTriangleMeshGeometry triangleMeshGeometry(triangleMesh);
        finishCreate(&triangleMeshGeometry, ignored);
        triangleMesh->release();
    }
}

MeshCollider::MeshCollider(ObjectCreationMode mode) : Super(mode), m_bConvex(), m_Mesh() {}

void MeshCollider::setMesh(Mesh *mesh) {
    m_Mesh = mesh;
    cleanup();
    create(nullptr);
}

void MeshCollider::setConvex(bool convex) {
    if (m_bConvex == convex) return;
    m_bConvex = convex;
    cleanup();
    create(nullptr);
}

void MeshCollider::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    Collider::onInspectorGUI();

    ItemLabel("Convex", kItemLabelLeft);
    bool bConvex = m_bConvex;
    if (ImGui::Checkbox("##Convex ", &bConvex)) {
        setConvex(bConvex);
    }

    ItemLabel("Mesh", kItemLabelLeft);
    std::string meshName = "None";
    if (m_Mesh) {
        meshName = m_Mesh->getName().string_view();
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
#endif
}


}// namespace AN