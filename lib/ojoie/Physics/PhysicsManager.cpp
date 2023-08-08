//
// Created by aojoie on 11/25/2022.
//

#include "Physics/PhysicsManager.hpp"
#include "Utility/Log.h"
#include "Core/Component.hpp"
#include "Core/Actor.hpp"
#include "Physics/RigidBody.hpp"
#include "Physics/Collider.hpp"
#include "Components/TransformComponent.hpp"

#include <PxPhysicsAPI.h>

#include <omnipvd/PxOmniPvd.h>
#include <OmniPvdWriter.h>
#include <OmniPvdFileWriteStream.h>

//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#define PVD_HOST "127.0.0.1"

using namespace physx;



namespace AN {

PxControllerManager *gPxControllerManager = nullptr;
PxPhysics           *gPxPhysics           = nullptr;
PxScene             *gPxScene             = nullptr;
PxMaterial          *gPxMaterial          = nullptr;
PxCooking           *gPxCooking           = nullptr;

struct PhysicsManager::Impl {
    PxFoundation*			foundation;
    PxPhysics*				physics;
    PxDefaultCpuDispatcher*	dispatcher;
    PxScene*			    scene;

#ifdef AN_DEBUG
    PxOmniPvd*					pvd;
#endif
    PxControllerManager* controllerManager;
};

PhysicsManager &PhysicsManager::GetSharedPhysics() {
    static PhysicsManager physics;
    return physics;
}

PhysicsManager::PhysicsManager() : impl(new Impl{}), m_Pause() {}

PhysicsManager::~PhysicsManager() {
    if (impl) {
        deinit();
        delete impl;
        impl = nullptr;
    }
}

void PhysicsManager::deinit() {

    /// destroy all RigidBody
    std::vector<RigidBody *> rigidBodies = Object::FindObjectsOfType<RigidBody>();
    for (RigidBody *body : rigidBodies) {
        DestroyObject(body);
    }

    /// destroy all colliders
    std::vector<Collider *> Colliders = Object::FindObjectsOfType<Collider>();
    for (Collider *body : Colliders) {
        DestroyObject(body);
    }

    if (impl && impl->foundation) {
        gPxCooking->release();
        impl->controllerManager->release();
        impl->scene->release();
        impl->dispatcher->release();
        impl->physics->release();
#ifdef AN_DEBUG
        impl->pvd->release();
#endif
        impl->foundation->release();

        impl->foundation = nullptr;
    }
}

class __PhysxErrorCallBack : public PxErrorCallback {

public:
    virtual void reportError(PxErrorCode::Enum code,
                             const char *message,
                             const char *file, int line) override {
        ANLog("Physx Report: [code]%d [msg]%s [file]%s [line]%d ",  code, message, file, line);
    }
};

bool PhysicsManager::init() {

    static __PhysxErrorCallBack gDefaultErrorCallback;
    static PxDefaultAllocator gDefaultAllocatorCallback;

    impl->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);

    if (!impl->foundation) {
        ANLog("PxCreateFoundation return null value");
        return false;
    }


#ifdef AN_DEBUG

    impl->pvd = PxCreateOmniPvd(*impl->foundation);
    if (impl->pvd) {
        OmniPvdWriter* omniWriter = impl->pvd->getWriter();
        // Uncomment for debugging the OmniPvd write stream
        //omniWriter->setLogFunction(logFunc);
        OmniPvdFileWriteStream* omniFileWriteStream = impl->pvd->getFileWriteStream();
        if (omniWriter && omniFileWriteStream) {
            omniWriter->setWriteStream(omniFileWriteStream);
        }

    } else {
        ANLog("PxCreateOmniPvd return null value");
    }

    impl->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *impl->foundation, PxTolerancesScale(),true, nullptr, impl->pvd);

#else

    impl->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *impl->foundation, PxTolerancesScale(),true, nullptr, nullptr);

#endif

    gPxPhysics = impl->physics;
#ifdef AN_DEBUG
    impl->pvd->getFileWriteStream()->setFileName("ojoie.ovd");
    impl->pvd->startSampling();
#endif

    PxSceneDesc sceneDesc(impl->physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    impl->dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher	= impl->dispatcher;
    sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
    impl->scene = impl->physics->createScene(sceneDesc);

    gPxScene = impl->scene;

//    gPxScene->setFlag(physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS, true);

#ifdef OJOIE_WITH_EDITOR
    gPxScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
    gPxScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);;
#endif//OJOIE_WITH_EDITOR

    PxPvdSceneClient* pvdClient = impl->scene->getScenePvdClient();

    if(pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }


    gPxMaterial = impl->physics->createMaterial(0.5f, 0.5f, 0.6f);


//    PxRigidStatic* groundPlane = PxCreatePlane(*impl->physics, PxPlane(0,1,0,0), *gPxMaterial);
//    impl->scene->addActor(*groundPlane);
//    groundPlane->setActorFlag(physx::PxActorFlag::eVISUALIZATION, false);

    impl->controllerManager = PxCreateControllerManager(*impl->scene);
    if (!impl->controllerManager) {
        ANLog("PxCreateControllerManager return null value");
        return false;
    }

    // global hook ref
    gPxControllerManager = impl->controllerManager;

    gPxCooking = PxCreateCooking(PX_PHYSICS_VERSION, *impl->foundation, PxCookingParams(impl->physics->getTolerancesScale()));
    if (!gPxCooking)
        AN_LOG(Error, "PxCreateCooking failed!");

    ANLog("Physx initialize success");

    return true;
}

void PhysicsManager::pause(bool v) {
    if (m_Pause == v) return;
    m_Pause = v;
}

void PhysicsManager::clearState() {
    PxU32 actorNum = gPxScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    std::vector<PxActor *> actors(actorNum);
    gPxScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), actors.size());
    for (PxActor *actor : actors) {
        PxRigidDynamic *rigidBody = actor->is<PxRigidDynamic>();
        if (rigidBody) {
            rigidBody->clearForce();
            rigidBody->clearTorque();
            rigidBody->setAngularVelocity({ 0.f, 0.f, 0.f });
            rigidBody->setLinearVelocity({ 0.f, 0.f, 0.f });
        }
    }
}

void PhysicsManager::update(float deltaTime) {
    if (m_Pause) return;

    impl->scene->simulate(deltaTime);
    impl->scene->fetchResults(true);

//    PxU32 count;
//    PxActor **activeActors = impl->scene->getActiveActors(count);

    PxU32 actorNum = gPxScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    std::vector<PxActor *> actors(actorNum);
    gPxScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), actors.size());

    for (int i = 0; i < actorNum; ++i) {
        PxActor *actor = actors[i];
        PxRigidBody *rigidBody = actor->is<PxRigidBody>();
        if (rigidBody) {
            PxTransform transform = rigidBody->getGlobalPose();
            RigidBody *rigidBodyCom = (RigidBody *)rigidBody->userData;

            Class::SetMessageEnabled(kDidChangeTransformMessage, false);
            rigidBodyCom->getTransform()->setPosition({ transform.p.x, transform.p.y, transform.p.z });
            rigidBodyCom->getTransform()->setRotation({ transform.q.w, transform.q.x, transform.q.y, transform.q.z }); /// ATTENTION w is first
            Class::SetMessageEnabled(kDidChangeTransformMessage, true);

            Message message;
            message.sender = this;
            message.name = kDidChangeTransformMessage;
            message.data = kAnimatePhysics;
            rigidBodyCom->getActor().sendMessage(message);
        }
    }
}


static const char *s_VisualizationShaderString = R"(Shader "Hidden/PhysicsVisualization"
{
    Properties
    {
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"
        #include "Lighting.hlsl"

        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            HLSLPROGRAM

            struct appdata
            {
                float3 vertex : POSITION;
            };
            struct v2f
            {
                float4 vertexOut : SV_POSITION;
            };
            v2f vertex_main(appdata v)
            {
                v2f o;
                o.vertexOut = mul(AN_MATRIX_VP, float4(v.vertex.xyz, 1.0));
                return o;
            }
            half4 fragment_main(v2f i) : SV_TARGET
            {
                return half4(0.f, 1.f, 0.f, 1.0);
            }
            ENDHLSL
        }
    }
}
)";

static Shader *s_VisualizationShader = nullptr;
static Material *s_VisualizationMaterial = nullptr;

void PhysicsManager::renderVisualization(CommandBuffer *commandBuffer) {

    if (s_VisualizationShader == nullptr) {
        s_VisualizationShader = NewObject<Shader>();
        s_VisualizationMaterial = NewObject<Material>();

        ANAssert(s_VisualizationShader->initWithScriptText(s_VisualizationShaderString));
        s_VisualizationShader->createGPUObject();

        s_VisualizationMaterial->init(s_VisualizationShader, "PhysicsVisualization");
    }

    const PxRenderBuffer &rb = impl->scene->getRenderBuffer();

    commandBuffer->debugLabelBegin("Render Physic Visualization", { 1.f, 1.f, 1.f, 1.f });

    s_VisualizationMaterial->applyMaterial(commandBuffer, 0U);

    commandBuffer->immediateBegin(kPrimitiveLines);

    for(PxU32 i=0; i < rb.getNbLines(); i++) {
        const PxDebugLine& line = rb.getLines()[i];
        Vector3f pos0{ line.pos0.x, line.pos0.y, line.pos0.z }, pos1{ line.pos1.x, line.pos1.y, line.pos1.z };

        commandBuffer->immediateVertex(pos0.x, pos0.y, pos0.z);
        commandBuffer->immediateVertex(pos1.x, pos1.y, pos1.z);
    }
    commandBuffer->immediateEnd();

    commandBuffer->debugLabelEnd();
}

}