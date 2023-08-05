//
// Created by Aleudillonam on 8/3/2023.
//

#include "Physics/BoxCollider.hpp"
#include <PxPhysicsAPI.h>

#ifdef OJOIE_WITH_EDITOR
#include "IMGUI/IMGUI.hpp"
#endif

using namespace physx;

namespace AN {

IMPLEMENT_AN_CLASS(BoxCollider)
LOAD_AN_CLASS(BoxCollider)

BoxCollider::~BoxCollider() {}

BoxCollider::BoxCollider(ObjectCreationMode mode) : Super(mode), m_Size(1.f) {}

void BoxCollider::create(RigidBody *ignored) {
    Vector3f size = m_Size * 0.5f;
    PxBoxGeometry geometry(size.x, size.y, size.z);
    finishCreate(&geometry, ignored);
}

void BoxCollider::setSize(const Vector3f &aSize) {
    m_Size = aSize;
    Vector3f size = m_Size * 0.5f;
    PxBoxGeometry geometry(size.x, size.y, size.z);
    PxShape *shape = (PxShape *)getShape();
    if (shape) {
        shape->setGeometry(geometry);
    }
}

void BoxCollider::onInspectorGUI() {
    Super::onInspectorGUI();
#ifdef OJOIE_WITH_EDITOR
    ItemLabel("Size", kItemLabelLeft);
    if (ImGui::DragFloat4("##SizeEdit", (float *)&m_Size, 0.01f, 0.01f, std::numeric_limits<float>::max())) {
        setSize(m_Size);
    }
#endif
}


}