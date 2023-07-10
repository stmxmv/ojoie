//
// Created by aojoie on 5/4/2023.
//

#ifndef OJOIE_CAMERA_HPP
#define OJOIE_CAMERA_HPP

#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Core/Component.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Render/RenderLoop/ForwardRenderLoop.hpp>
#include <ojoie/Render/Renderer.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/UniformBuffers.hpp>
#include <ojoie/Template/LinkedList.hpp>
#include <ojoie/Render/RenderTarget.hpp>

namespace AN {

class Camera;
typedef ListNode<Camera> CameraListNode;
typedef List<CameraListNode> CameraList;

class AN_API Camera final : public Component {

    RenderTarget *_renderTarget;
    CameraListNode _cameraListNode;

    bool bMatchLayerRatio;

    std::unique_ptr<RenderLoop> _renderLoop;

    float _fovyDegree{ 60.f }, _nearZ{ 0.03f }, _farZ{ 10000.f };
    float viewportRatio{ 1.f };

    DECLARE_DERIVED_AN_CLASS(Camera, Component);

    void beginRender(RenderContext &renderContext);

public:

    static bool IsSealedClass() { return true; }

    static void InitializeClass();

    explicit Camera(ObjectCreationMode mode);

    virtual bool init() override;

    virtual void dealloc() override;

    void update(UInt32 frameIndex);

    void setViewportRatio(float ratio) { viewportRatio = ratio; }

    bool isMatchLayerRatio() const { return bMatchLayerRatio; }
    void setMatchLayerRatio(bool match) { bMatchLayerRatio = match; }

    RenderTarget *getRenderTarget() const { return _renderTarget; }

    void setRenderTarget(RenderTarget *renderTarget) {
        _renderTarget = renderTarget;
        _renderLoop->setRenderTarget(renderTarget);
    }

    void drawRenderers(RenderContext &context, const RendererList &rendererList);

//    const Matrix4x4f &getProjectionMatrix() const { return proj; }
//    const Matrix4x4f &getInverseProjectionMatrix() const { return inProj; }
//
//    const Matrix4x4f &getViewMatrix() const { return view; }
//    const Matrix4x4f &getInverseViewMatrix() const { return inView; }

};

class AN_API CameraManager {

    CameraList _cameraList;

public:

    void addCamera(CameraListNode &camera);
    void removeCamera(CameraListNode &camera);

    void updateCameras(UInt32 frameIndex);

    CameraList  &getCameras() { return _cameraList; }

};

AN_API CameraManager &GetCameraManager();

}

#endif//OJOIE_CAMERA_HPP
