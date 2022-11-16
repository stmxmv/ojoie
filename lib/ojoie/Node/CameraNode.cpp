//
// Created by Aleudillonam on 8/4/2022.
//

#include "Node/CameraNode.hpp"
#include "Render/RenderQueue.hpp"
#include "Render/Scene.hpp"

namespace AN {

static std::weak_ptr<CameraNode> r_current_camera_node;

std::shared_ptr<class CameraNode> Node3D::GetCurrentCamera() {
    return r_current_camera_node.lock();
}

void Node3D::SetCurrentCamera(const std::shared_ptr<class CameraNode> &cameraNode) {
    r_current_camera_node = cameraNode;
}


void CameraNode::CameraNodeSceneProxy::render(const RenderContext &context) {
    Super::SceneProxyType::render(context);

    if (viewportWidth <= 0.f || viewportHeight <= 0.f) {
        if (frameWidth != context.frameWidth || frameHeight != context.frameHeight) {
            frameWidth = context.frameWidth;
            frameHeight = context.frameHeight;
        }
    }

    preView = view;

    view = getInverseModelViewMatrix();

    preProjection = projection;
    projection = Math::perspective(Math::radians(fovyDegree),
                                             (float)(frameWidth / frameHeight),
                                             nearZ, farZ);
#if defined(OJOIE_USE_GLM) && defined(OJOIE_USE_VULKAN)
    projection[1][1] *= -1;
#endif

    getScene().setProjectionMatrix(projection);
    getScene().setViewMatrix(view);
//    SetCurrentCamera(std::static_pointer_cast<CameraNode>(shared_from_this()));
}

void CameraNode::updateSceneProxy() {
    Super::updateSceneProxy();

    if (didChangeProjection) {

        struct Param {
            float fovyDegree, nearZ, farZ;
            float viewportWidth, viewportHeight;
        } param;

        param.fovyDegree = _fovyDegree;
        param.nearZ = _nearZ;
        param.farZ = _farZ;
        param.viewportWidth = viewportWidth;
        param.viewportHeight = viewportHeight;

        sceneProxy->retain();
        GetRenderQueue().enqueue([param, sceneProxy = sceneProxy] {
            auto *proxy = (CameraNodeSceneProxy *)sceneProxy;

            proxy->fovyDegree = param.fovyDegree;
            proxy->nearZ = param.nearZ;
            proxy->farZ = param.farZ;
            proxy->viewportWidth = param.viewportWidth;
            proxy->viewportHeight = param.viewportHeight;

            if (proxy->viewportWidth > 0.f || proxy->viewportHeight > 0.f) {
                proxy->frameWidth = proxy->viewportWidth;
                proxy->frameHeight = proxy->viewportHeight;
            }

            proxy->release();
        });

        didChangeProjection = false;
    }
}
}