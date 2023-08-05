//
// Created by Aleudillonam on 8/3/2023.
//

#include "Physics/Collider.hpp"
#include "Physics/RigidBody.hpp"
#include "Core/Actor.hpp"
#include "Components/TransformComponent.hpp"
#include <PxPhysicsAPI.h>

#ifdef OJOIE_WITH_EDITOR
#include "IMGUI/IMGUI.hpp"
#endif

namespace AN {


IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(Collider)
LOAD_AN_CLASS(Collider)


using namespace physx;
extern PxPhysics  *gPxPhysics;
extern PxScene    *gPxScene;
extern PxMaterial *gPxMaterial;

struct Collider::Impl {
    PxShape *shape;
};

void Collider::OnAddComponentMessage(void *receiver, Message &message) {
    Collider *self = (Collider *)receiver;
    Component *component = message.getData<Component *>();
    if (component->is<RigidBody>()) {
        PxRigidActor *rigidActor = self->impl->shape->getActor();
        if (rigidActor) {
            rigidActor->detachShape(*self->impl->shape);

            if (rigidActor->userData == nullptr) {
                gPxScene->removeActor(*rigidActor);
                rigidActor->release();

                /// recreate shape
                self->impl->shape->release();
                self->create(nullptr);
                return;
            }
        }

        /// recreate shape
        self->impl->shape->release();
        self->create(nullptr);
    }
}

void Collider::OnRemoveRigidBodyMessage(void *receiver, Message &message) {
    Collider *self = (Collider *)receiver;
    Component *component = message.getData<Component *>();
    if (component->is<RigidBody>()) {
        PxRigidActor *rigidActor = (PxRigidActor *)component->as<RigidBody>()->getUnderlyingObject();
        PxRigidActor *actor = self->impl->shape->getActor();
        if (rigidActor == actor) {
            actor->detachShape(*self->impl->shape);

            /// recreate shape
            self->impl->shape->release();
            self->create(component->as<RigidBody>());
        }
    }
}

void Collider::OnTransformChangeMessage(void *receiver, Message &message) {
    Collider *self = (Collider *)receiver;
    TransformChangeFlags flag = message.getData<TransformChangeFlags>();
    if (flag & (kPositionChanged | kRotationChanged)) {
        PxRigidActor *rigidActor = self->impl->shape->getActor();
        if (rigidActor && rigidActor->userData == nullptr) {
            /// static
            Vector3f    position = self->getTransform()->getPosition();
            Quaternionf rotation = self->getTransform()->getRotation();
            PxTransform transform;
            transform.p = PxVec3(position.x, position.y, position.z);
            transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
            rigidActor->setGlobalPose(transform);
        }
    }
}

void Collider::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage, OnAddComponentMessage);
    GetClassStatic()->registerMessageCallback(kDidChangeTransformMessage, OnTransformChangeMessage);
    GetClassStatic()->registerMessageCallback(kWillRemoveRigidBodyMessage, OnRemoveRigidBodyMessage);
}

Collider::Collider(ObjectCreationMode mode) : Super(mode), impl(new Impl()) {}

Collider::~Collider() {
    delete impl;
}

void Collider::finishCreate(void *shapeDesc, RigidBody *ignored) {
    PxGeometry *geometry = (PxGeometry *) shapeDesc;
    impl->shape          = gPxPhysics->createShape(*geometry, *gPxMaterial, true);

#ifdef OJOIE_WITH_EDITOR
    impl->shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
#endif

    RigidBody *rigidBody = getComponent<RigidBody>();
    if (rigidBody && rigidBody != ignored) {
        PxRigidActor *actor = (PxRigidActor *)rigidBody->getUnderlyingObject();
        actor->attachShape(*impl->shape);

        ANAssert(impl->shape->getActor() == actor);

    } else {

        Vector3f    position = getTransform()->getPosition();
        Quaternionf rotation = getTransform()->getRotation();
        PxTransform transform;
        transform.p = PxVec3(position.x, position.y, position.z);
        transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);

        PxRigidStatic *rigidStatic = gPxPhysics->createRigidStatic(transform);
        rigidStatic->attachShape(*impl->shape);
        gPxScene->addActor(*rigidStatic);
    }
}

void Collider::dealloc() {

    if (impl->shape->getActor() && impl->shape->getActor()->userData == nullptr) {
        impl->shape->getActor()->release();
    }

    if (impl->shape->getActor()) {
        PxRigidActor *actor = impl->shape->getActor();
        actor->detachShape(*impl->shape);
        PxRigidDynamic *rigidDynamic = actor->is<PxRigidDynamic>();
        if (rigidDynamic) {
            rigidDynamic->wakeUp();
        }
    }

    impl->shape->release();
    Super::dealloc();
}

bool Collider::init() {
    if (!Super::init()) return false;
    create(nullptr);
    return true;
}

void *Collider::getShape() {
    return impl->shape;
}

void Collider::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    if (!ImGui::IsWindowFocused()) {
        if (m_Visualization) {
            impl->shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
            m_Visualization = false;
        }
    }

    ItemLabel("Visualization", kItemLabelLeft);

    if (ImGui::Checkbox("##VisualizationCheckbox", &m_Visualization)) {
        impl->shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, m_Visualization);
    }

#endif
}

}// namespace AN