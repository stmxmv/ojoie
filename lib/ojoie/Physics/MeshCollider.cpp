//
// Created by Aleudillonam on 8/4/2023.
//

#include "Physics/MeshCollider.hpp"
#include <PxPhysicsAPI.h>


namespace AN {

IMPLEMENT_AN_CLASS(MeshCollider)
//LOAD_AN_CLASS(MeshCollider) /// TODO

using namespace physx;
extern PxPhysics  *gPxPhysics;
extern PxScene    *gPxScene;
extern PxMaterial *gPxMaterial;


MeshCollider::~MeshCollider() {
}

void MeshCollider::create(RigidBody *ignored) {
    if (!m_Mesh) return;

    /// TODO
//    PxTriangleMeshDesc meshDesc;
//    meshDesc.points.count           = nbVerts;
//    meshDesc.points.stride          = sizeof(PxVec3);
//    meshDesc.points.data            = verts;
//
//    meshDesc.triangles.count        = triCount;
//    meshDesc.triangles.stride       = 3*sizeof(PxU32);
//    meshDesc.triangles.data         = indices32;
//
//    PxDefaultMemoryOutputStream writeBuffer;
//    PxTriangleMeshCookingResult::Enum result;
//    bool status = cooking.cookTriangleMesh(meshDesc, writeBuffer,result);
//    if(!status)
//        return NULL;
//
//    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
//    return physics.createTriangleMesh(readBuffer);

}
MeshCollider::MeshCollider(ObjectCreationMode mode) : Super(mode), m_Mesh() {
}
void MeshCollider::onInspectorGUI() {
    Collider::onInspectorGUI();
}


}