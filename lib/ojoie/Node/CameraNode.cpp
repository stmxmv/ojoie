//
// Created by Aleudillonam on 8/4/2022.
//

#include "Node/CameraNode.hpp"
#include "Render/RenderPipeline.hpp"

namespace AN {

static std::weak_ptr<CameraNode> r_current_camera_node;

std::shared_ptr<class CameraNode> Node3D::GetCurrentCamera() {
    return r_current_camera_node.lock();
}

void Node3D::SetCurrentCamera(const std::shared_ptr<class CameraNode> &cameraNode) {
    r_current_camera_node = cameraNode;
}

void CameraNode::render(const RenderContext &context) {
    Super::render(context);
    bool shouldUpdateProjection = false;
    if (r_didChangeProjection) {
        r_didChangeProjection = false;
        shouldUpdateProjection = true;
    }

    if (r_frameWidth != context.frameWidth || r_frameHeight != context.frameHeight) {
        shouldUpdateProjection = true;
        r_frameWidth = context.frameWidth;
        r_frameHeight = context.frameHeight;
    }

    if (shouldUpdateProjection) {
        r_projection = Math::perspective(Math::radians(r_fovyDegree), (float)(context.frameWidth / context.frameHeight), r_nearZ, r_farZ);
    }

    SetCurrentCamera(std::static_pointer_cast<CameraNode>(shared_from_this()));
}

}