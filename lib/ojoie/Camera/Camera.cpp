//
// Created by aojoie on 5/5/2023.
//

#include "Camera/Camera.hpp"
#include "Components/TransformComponent.hpp"

#include "Core/Actor.hpp"
#include "Render/Material.hpp"
#include "Render/VertexBuffer.hpp"
#include "Render/RenderManager.hpp"
#include "Misc/ResourceManager.hpp"
#include "Geometry/Sphere.hpp"
#include "Render/TextureCube.hpp"

namespace AN {

static constexpr int ANGlobalUniformBufferSize = 400;


IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(Camera);
LOAD_AN_CLASS(Camera);

Camera::~Camera() {}

Camera::Camera(ObjectCreationMode mode)
    : Super(mode), _renderTarget(), _cameraListNode(this), bMatchLayerRatio(true) {}

static const UInt8 skyboxVertices[] = {
#include "SkyboxMesh.inl"
};

static constexpr int kSkyboxVertexCount = sizeof(skyboxVertices) / sizeof(Vector3f);
static std::unique_ptr<VertexBuffer> skyboxVertexBuffer;

void Camera::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage,
                                              [](void *receiver, Message &message) {
                                                  Camera *camera = (Camera *) receiver;
                                                  if (message.getData<Component *>() == camera) {
                                                      /// only do when the added component is self
                                                      GetCameraManager().addCamera(camera->_cameraListNode);
                                                  }
                                              });

    skyboxVertexBuffer = std::make_unique<VertexBuffer>();
    skyboxVertexBuffer->init();

    GetRenderManager().registerInitializeTask([] {
        VertexBufferData vertexBufferData{};
        vertexBufferData.streams[0].stride      = sizeof(Vector3f);
        vertexBufferData.streams[0].channelMask = VERTEX_FORMAT1(Vertex);
        vertexBufferData.streams[0].offset      = 0;

        vertexBufferData.channels[kShaderChannelVertex].format    = kChannelFormatFloat;
        vertexBufferData.channels[kShaderChannelVertex].dimension = 3;
        vertexBufferData.channels[kShaderChannelVertex].offset    = 0;
        vertexBufferData.channels[kShaderChannelVertex].stream    = 0;

        vertexBufferData.buffer      = skyboxVertices;
        vertexBufferData.bufferSize  = sizeof(skyboxVertices);
        vertexBufferData.vertexCount = kSkyboxVertexCount;

        skyboxVertexBuffer->updateVertexData(vertexBufferData);
    });

    GetRenderManager().registerCleanupTask([] {
        skyboxVertexBuffer.reset();
    });

    /// setting default param
    Material::SetVectorGlobal("_ProjectionParams", {});
    Material::SetMatrixGlobal("an_MatrixV", Math::identity<Matrix4x4f>());
    Material::SetMatrixGlobal("an_MatrixInvV", Math::identity<Matrix4x4f>());
    Material::SetMatrixGlobal("an_MatrixP", Math::identity<Matrix4x4f>());
    Material::SetMatrixGlobal("an_MatrixInvP", Math::identity<Matrix4x4f>());
    Material::SetMatrixGlobal("an_MatrixVP", Math::identity<Matrix4x4f>());
    Material::SetMatrixGlobal("an_MatrixInvVP", Math::identity<Matrix4x4f>());
}

void Camera::update(UInt32 frameIndex) {
    TransformComponent *transform = getTransform();

    if (transform) {
        an_MatrixV = transform->getWorldToLocalMatrix();
        an_MatrixInvV = transform->getLocalToWorldMatrix();
    } else {
        an_MatrixV = Matrix4x4f(1.f);
        an_MatrixInvV = Matrix4x4f(1.f);
    }

    an_MatrixP = Math::perspective(Math::radians(_fovyDegree), viewportRatio, _nearZ, _farZ);

#if defined(OJOIE_USE_GLM) && defined(OJOIE_USE_VULKAN)

    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
        an_MatrixP[1][1] *= -1;
    }

#endif

    an_MatrixInvP = Math::inverse(an_MatrixP);
    an_MatrixVP = an_MatrixP * an_MatrixV;
    an_MatrixInvVP = Math::inverse(an_MatrixVP);

    _ProjectionParams = { 1.f, _nearZ, _farZ, 1.f / _farZ };
    _renderLoop->performUpdate(frameIndex);
}

void Camera::drawSkyBox(RenderContext &renderContext) {
    static bool proceduralSkyBox = true;

    if (proceduralSkyBox) {
        static Material* s_SkyBoxMaterial = NULL;
        static Shader* s_SkyBoxShader = NULL;
        if (!s_SkyBoxMaterial) {
            s_SkyBoxShader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "Skybox-Procedural");
            ANAssert(s_SkyBoxShader);
            s_SkyBoxMaterial = NewObject<Material>();
            ANAssert(s_SkyBoxMaterial->init(s_SkyBoxShader, "SkyBoxMaterial"));
            s_SkyBoxMaterial->setFloat("_Exposure", 1.25f);
            s_SkyBoxMaterial->setVector("_GroundColor", { 0.46f, 0.45f, 0.46f, 1.f });
            s_SkyBoxMaterial->setFloat("_SunSize", 0.04f);
            s_SkyBoxMaterial->setFloat("_SunSizeConvergence", 5);
            s_SkyBoxMaterial->setVector("_SkyTint", { 1.f, 1.f, 1.f, 1.f });
            s_SkyBoxMaterial->setFloat("_AtmosphereThickness", 0.53f);
        }

        s_SkyBoxMaterial->setVector("an_WorldTransformParams", { 0, 0, 0, 1.f });
        Vector3f position = getTransform()->getTransform()->getLocalPosition();
        Matrix4x4f objectToWorld = Math::translate(position) * Math::scale(Vector3f(_farZ * 0.9f));
        s_SkyBoxMaterial->setMatrix("an_ObjectToWorld", objectToWorld);
        s_SkyBoxMaterial->setMatrix("an_WorldToObject", Math::inverse(objectToWorld));

        CommandBuffer *commandBuffer = renderContext.commandBuffer;
        s_SkyBoxMaterial->applyMaterial(commandBuffer, 0U);

        //    commandBuffer->immediateBegin(kPrimitiveQuads);
        //    commandBuffer->immediateVertex (-1.f, -1.f, -1.f);
        //    commandBuffer->immediateVertex (1.f, -1.f, -1.f);
        //    commandBuffer->immediateVertex (1.f, 1.f, -1.f);
        //    commandBuffer->immediateVertex (-1.f, 1.f, -1.f);
        //    commandBuffer->immediateEnd ();

        skyboxVertexBuffer->draw(commandBuffer, kSkyboxVertexCount);

        //        commandBuffer->immediateBegin(kPrimitiveTriangles);
        //        for (UInt32 index : sphere.indices) {
        //            commandBuffer->immediateVertex(sphere.vertices[index].x, sphere.vertices[index].y, sphere.vertices[index].z);
        //
        //        }
        //        commandBuffer->immediateEnd ();

        //            commandBuffer->immediateBegin(kPrimitiveQuads);
        //            commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        //            commandBuffer->immediateVertex(-1.f, 1.f, -1.f);
        //
        //            commandBuffer->immediateVertex(-1.f, -1.f, 1.f);
        //            commandBuffer->immediateVertex(1.f, -1.f, 1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        //            commandBuffer->immediateVertex(-1.f, 1.f, 1.f);
        //
        //            commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(-1.f, 1.f, -1.f);
        //            commandBuffer->immediateVertex(-1.f, 1.f, 1.f);
        //            commandBuffer->immediateVertex(-1.f, -1.f, 1.f);
        //
        //            commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        //            commandBuffer->immediateVertex(1.f, -1.f, 1.f);
        //
        //            commandBuffer->immediateVertex(-1.f, 1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        //            commandBuffer->immediateVertex(-1.f, 1.f, 1.f);
        //
        //            commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        //            commandBuffer->immediateVertex(1.f, -1.f, 1.f);
        //            commandBuffer->immediateVertex(-1.f, -1.f, 1.f);
        //
        //            commandBuffer->immediateEnd ();
    } else {

        static Material* s_SkyBoxMaterial = NULL;
        static TextureCube *cubeMap = nullptr;
        static Shader* s_SkyBoxShader = NULL;
        if (!s_SkyBoxMaterial) {
            s_SkyBoxShader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "Skybox");
            ANAssert(s_SkyBoxShader);
            s_SkyBoxMaterial = NewObject<Material>();
            ANAssert(s_SkyBoxMaterial->init(s_SkyBoxShader, "SkyBoxMaterial"));

            cubeMap = (TextureCube *)GetResourceManager().getResource(TextureCube::GetClassNameStatic(), "Skybox");
            s_SkyBoxMaterial->setTexture("_MainTex", cubeMap);
        }

        s_SkyBoxMaterial->setVector("an_WorldTransformParams", { 0, 0, 0, 1.f });
        Vector3f   position      = getTransform()->getTransform()->getLocalPosition();
        Matrix4x4f objectToWorld = Math::translate(position);
        s_SkyBoxMaterial->setMatrix("an_ObjectToWorld", objectToWorld);
        s_SkyBoxMaterial->setMatrix("an_WorldToObject", Math::inverse(objectToWorld));

        CommandBuffer *commandBuffer = renderContext.commandBuffer;
        s_SkyBoxMaterial->applyMaterial(commandBuffer, 0U);

        commandBuffer->immediateBegin(kPrimitiveQuads);
        commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        commandBuffer->immediateVertex(-1.f, 1.f, -1.f);

        commandBuffer->immediateVertex(-1.f, -1.f, 1.f);
        commandBuffer->immediateVertex(1.f, -1.f, 1.f);
        commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        commandBuffer->immediateVertex(-1.f, 1.f, 1.f);

        commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(-1.f, 1.f, -1.f);
        commandBuffer->immediateVertex(-1.f, 1.f, 1.f);
        commandBuffer->immediateVertex(-1.f, -1.f, 1.f);

        commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        commandBuffer->immediateVertex(1.f, -1.f, 1.f);

        commandBuffer->immediateVertex(-1.f, 1.f, -1.f);
        commandBuffer->immediateVertex(1.f, 1.f, -1.f);
        commandBuffer->immediateVertex(1.f, 1.f, 1.f);
        commandBuffer->immediateVertex(-1.f, 1.f, 1.f);

        commandBuffer->immediateVertex(-1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(1.f, -1.f, -1.f);
        commandBuffer->immediateVertex(1.f, -1.f, 1.f);
        commandBuffer->immediateVertex(-1.f, -1.f, 1.f);

        commandBuffer->immediateEnd();
    }
}

void Camera::beginRender() {
    Material::SetVectorGlobal("_ProjectionParams", _ProjectionParams);
    Material::SetMatrixGlobal("an_MatrixV", an_MatrixV);
    Material::SetMatrixGlobal("an_MatrixInvV", an_MatrixInvV);
    Material::SetMatrixGlobal("an_MatrixP", an_MatrixP);
    Material::SetMatrixGlobal("an_MatrixInvP", an_MatrixInvP);
    Material::SetMatrixGlobal("an_MatrixVP", an_MatrixVP);
    Material::SetMatrixGlobal("an_MatrixInvVP", an_MatrixInvVP);
}

bool Camera::init() {
    if (!Super::init()) return false;

    _renderLoop = std::make_unique<ForwardRenderLoop>();

    return _renderLoop->init();
}

void Camera::dealloc() {
    _renderLoop->deinit();
    _renderLoop.reset();
    Super::dealloc();
}

void Camera::drawRenderers(RenderContext &context, const RendererList &rendererList) {
    struct Param {
        Camera *self;
        const RendererList &rendererList;
    } param{ this, rendererList };

    _renderLoop->performRender(context,
            [](RenderContext &renderContext, void *userdata) {
              Param *param = (Param *)userdata;

              param->self->beginRender();

              for (auto &node : param->rendererList) {
                  Renderer &renderer = *node;
                  renderer.render(renderContext, "Forward");
              }

              param->self->drawSkyBox(renderContext);
    }, &param);

}

void CameraManager::addCamera(CameraListNode &camera) {
    _cameraList.push_back(camera);
}

void CameraManager::removeCamera(CameraListNode &camera) {
    camera.removeFromList();
}

void CameraManager::updateCameras(UInt32 frameIndex) {
    for (auto &node : _cameraList) {
        Camera &camera = *node;
        camera.update(frameIndex);
    }
}

CameraManager &GetCameraManager() {
    static CameraManager cameraManager;
    return cameraManager;
}

}