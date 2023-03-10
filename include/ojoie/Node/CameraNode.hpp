//
// Created by Aleudillonam on 8/4/2022.
//

#ifndef OJOIE_CAMERA_HPP
#define OJOIE_CAMERA_HPP

#include <ojoie/Node/Node3D.hpp>

namespace AN {


class CameraNode : public Node3D {
    typedef CameraNode Self;
    typedef Node3D Super;

    float _fovyDegree{ 60.f }, _nearZ{ 1.f }, _farZ{ 1000000.f };

    float viewportWidth{}, viewportHeight{};

    bool didChangeProjection{ true };

public:

    struct CameraNodeSceneProxy : Super::SceneProxyType {

        float fovyDegree, nearZ, farZ;

        float viewportWidth, viewportHeight;

        float frameWidth{}, frameHeight{};
        Math::mat4 projection, view, preProjection{ 1.f }, preView{ 1.f };

        explicit CameraNodeSceneProxy(CameraNode &node) : Node3DSceneProxy(node) {
            fovyDegree = node._fovyDegree;
            nearZ = node._nearZ;
            farZ = node._farZ;
            viewportWidth = node.viewportWidth;
            viewportHeight = node.viewportHeight;
        }


        virtual void render(const RenderContext &context) override;

    };

    typedef CameraNodeSceneProxy SceneProxyType;

    CameraNode() {
        _needsRender = true;
    }

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


    virtual RC::SceneProxy *createSceneProxy() override {
        return new CameraNodeSceneProxy(*this);
    }

    virtual void updateSceneProxy() override;

    void setZoom(float zoom) {
        _fovyDegree = zoom;
        didChangeProjection = true;
    }

    void setProjection(float fovyDegree, float nearZ, float farZ) {
        _fovyDegree = fovyDegree;
        _nearZ = nearZ;
        _farZ = farZ;
        didChangeProjection = true;
    }

    void setViewportSize(float width, float height) {
        viewportWidth = width;
        viewportHeight = height;
        didChangeProjection = true;
    }

    float getFovyDegree() const {
        return _fovyDegree;
    }

    float getNearZ() const {
        return _nearZ;
    }

    float getFarZ() const {
        return _farZ;
    }

    static Math::vec3 GetDefaultForwardVector() {
        return { 0.f, 0.f, -1.f };
    }

    static Math::vec3 GetDefaultRightVector() {
        return { 1.f, 0.f, 0.f };
    }
};



}


#endif//OJOIE_CAMERA_HPP
