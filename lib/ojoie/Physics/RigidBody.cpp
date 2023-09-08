//
// Created by Aleudillonam on 8/3/2023.
//

#include "Physics/RigidBody.hpp"
#include "Components/Transform.hpp"
#include "Core/Actor.hpp"
#include <PxPhysicsAPI.h>

#ifdef OJOIE_WITH_EDITOR
#include "IMGUI/IMGUI.hpp"
#endif

namespace AN {

IMPLEMENT_AN_CLASS_INIT(RigidBody)
LOAD_AN_CLASS(RigidBody)

AN_API const Name kWillRemoveRigidBodyMessage("WillRemoveRigidBody");

using namespace physx;

extern PxPhysics *gPxPhysics;
extern PxScene   *gPxScene;

struct RigidBody::Impl {
    physx::PxRigidDynamic *body;
};

void RigidBody::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidChangeTransformMessage, OnTransformChangedMessage);
}

void RigidBody::OnTransformChangedMessage(void *receiver, Message &message) {
    RigidBody *rigidBody = (RigidBody *)receiver;
    TransformChangeFlags flag = message.getData<TransformChangeFlags>();
    if (flag & (kPositionChanged | kRotationChanged)) {
        Vector3f    position = rigidBody->getTransform()->getPosition();
        Quaternionf rotation = rigidBody->getTransform()->getRotation();
        PxTransform transform;
        transform.p = PxVec3(position.x, position.y, position.z);
        transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
        rigidBody->impl->body->setGlobalPose(transform);
    }
}

RigidBody::RigidBody(ObjectCreationMode mode) : Super(mode), impl(new Impl()) {}

RigidBody::~RigidBody() {
    delete impl;
}


void RigidBody::dealloc() {

    Message message;
    message.sender = this;
    message.data = (intptr_t)this;
    message.name = kWillRemoveRigidBodyMessage;
    getActor().sendMessage(message);

    if (m_AddToScene) {
        gPxScene->removeActor(*impl->body);
    }

    impl->body->release();

    Super::dealloc();
}

bool RigidBody::init() {
    if (!Super::init()) return false;

    Vector3f    position = getTransform()->getPosition();
    Quaternionf rotation = getTransform()->getRotation();

    PxTransform transform;
    transform.p = PxVec3(position.x, position.y, position.z);
    transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);

    impl->body = gPxPhysics->createRigidDynamic(transform);

    m_Mass = 1.f;// default mass

    impl->body->setMass(m_Mass);

    gPxScene->addActor(*impl->body);
    m_AddToScene = true;
    m_Drag = 0;
    m_UseGravity = true;
    impl->body->userData = this;

    return true;
}

void *RigidBody::getUnderlyingObject() {
    return impl->body;
}

void RigidBody::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    ItemLabel("Mass", kItemLabelLeft);
    if (ImGui::DragFloat("##MassEdit", &m_Mass, 0.01f, 0.0f, std::numeric_limits<float>::max())) {
        impl->body->setMass(m_Mass);
    }

    ItemLabel("Drag", kItemLabelLeft);
    if (ImGui::DragFloat("##DragEdit", &m_Drag, 0.01f, 0.0f, std::numeric_limits<float>::max())) {
        impl->body->setLinearDamping(m_Drag);
    }

    ItemLabel("Use Gravity", kItemLabelLeft);
    if (ImGui::Checkbox("##GravityCheckbox", &m_UseGravity)) {
        impl->body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !m_UseGravity);
    }
#endif
}


}// namespace AN