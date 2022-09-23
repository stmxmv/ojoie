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

    float _fovyDegree{ 45.f }, _nearZ{ 1.f }, _farZ{ 10000.f };



    bool r_didChangeProjection{ true };
    float r_fovyDegree{ 45.f }, r_nearZ{ 1.f }, r_farZ{ 10000.f };
    Math::mat4 r_projection, r_view, preProjection{ 1.f }, preView{ 1.f };
    float r_frameWidth{}, r_frameHeight{};
public:

    CameraNode() {
        _needsRender = true;
    }

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual void render(const RenderContext &context) override;

    void setZoom(float zoom) {
        _fovyDegree = zoom;
        Dispatch::async(Dispatch::Render, [=, weakSelf = weak_from_this()] {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *)_self.get();
                self->r_fovyDegree = zoom;
                self->r_didChangeProjection = true;
            }
        });
    }

    void setProjection(float fovyDegree, float nearZ, float farZ) {
        _fovyDegree = fovyDegree;
        _nearZ = nearZ;
        _farZ = farZ;
        Dispatch::async(Dispatch::Render, [=, weakSelf = weak_from_this()] {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *)_self.get();
                self->r_fovyDegree = fovyDegree;
                self->r_nearZ = nearZ;
                self->r_farZ = farZ;
                self->r_didChangeProjection = true;
            }
        });
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

    /////////////////// renderer's method
    Math::mat4 getProjectionMatrix() const {
        return r_projection;
    }

    Math::mat4 getViewMatrix() const {
        return r_view;
    }

    Math::mat4 getPreProjection() const {
        return preProjection;
    }

    Math::mat4 getPreView() const {
        return preView;
    }
};



}


#endif//OJOIE_CAMERA_HPP
