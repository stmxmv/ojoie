//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_3DNODE_HPP
#define OJOIE_3DNODE_HPP

#include <ojoie/Core/Dispatch.hpp>
#include <ojoie/Core/Node.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {


/// \brief a node3d has a position in the 3d world
class Node3D : public Node {
    typedef Node3D Self;
    typedef Node Super;


    Math::vec3 _position{};
    Math::vec3 _scale{ 1.f };
    Math::rotator _rotation{};

    std::atomic_bool didSetTransform{ true };

    /// render's data
    Math::vec3 r_position{};
    Math::vec3 r_scale{ 1.f };
    Math::rotator r_rotation{};

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);
        if (didSetTransform.load(std::memory_order_relaxed)) {
            didSetTransform.store(false, std::memory_order_relaxed);
            Dispatch::async(Dispatch::Render, [position = _position, scale = _scale, rotation = _rotation, weakSelf = weak_from_this()] {
                auto _self = weakSelf.lock();
                if (_self) {
                    Self *self = (Self *) _self.get();
                    self->r_position = position;
                    self->r_scale = scale;
                    self->r_rotation = rotation;
                }
            });
        }
    }

    const Math::vec3 &getPosition() const {
        return _position;
    }

    const Math::vec3 &getScale() const {
        return _scale;
    }

    const Math::rotator &getRotation() const {
        return _rotation;
    }

    void setPosition(const Math::vec3 &position) {
        _position = position;
        if (!tick) {
            Dispatch::async(Dispatch::Render, [position = _position, weakSelf = weak_from_this()] {
                auto _self = weakSelf.lock();
                if (_self) {
                    Self *self = (Self *) _self.get();
                    self->r_position = position;
                }
            });
        } else {
            didSetTransform = true;
        }
    }

    void setScale(const Math::vec3 &scale) {
        _scale = scale;
        if (!tick) {
            Dispatch::async(Dispatch::Render, [scale, weakSelf = weak_from_this()] {
                auto _self = weakSelf.lock();
                if (_self) {
                    Self *self = (Self *) _self.get();
                    self->r_scale = scale;
                }
            });
        } else {
            didSetTransform = true;
        }
    }

    void setRotation(const Math::rotator &rotation) {
        _rotation = rotation;
        if (!tick) {
            Dispatch::async(Dispatch::Render, [rotation, weakSelf = weak_from_this()] {
                auto _self = weakSelf.lock();
                if (_self) {
                    Self *self = (Self *) _self.get();
                    self->r_rotation = rotation;
                }
            });
        } else {
            didSetTransform = true;
        }
    }

    /////////////////////////////// render's method

    static std::shared_ptr<class CameraNode> GetCurrentCamera();

    static void SetCurrentCamera(const std::shared_ptr<class CameraNode> &cameraNode);

    Math::mat4 getModelViewMatrix() {
        Math::mat4 translationMatrix = Math::translate(Math::mat4(1.f), r_position);
        Math::mat4 scaleMatrix = Math::scale(Math::mat4(1.f), r_scale);
        Math::mat4 rotationMatrix = Math::eulerAngleYXZ(Math::radians(r_rotation.yaw), Math::radians(r_rotation.pitch), Math::radians(r_rotation.roll));
        Math::mat4 modelViewMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        auto par = parent.lock();
        if (par) {
            auto par3d = std::dynamic_pointer_cast<Node3D>(par);
            if (par3d) {
                return par3d->getModelViewMatrix() * modelViewMatrix;
            }
        }

        return modelViewMatrix;
    }
    
    Math::mat4 getInverseModelViewMatrix() {
        Math::mat4 translationInverseMatrix = Math::translate(Math::mat4(1.f), -r_position);
        Math::mat4 scaleInverseMatrix = Math::scale(Math::mat4(1.f), 1.f / r_scale);
//        Math::mat4 rotationInverseMatrix = Math::transpose(Math::eulerAngleYXZ(r_rotation.yaw, r_rotation.pitch, r_rotation.row));
        Math::mat4 rotationInverseMatrix =  Math::eulerAngleZXY(Math::radians(-r_rotation.roll), Math::radians(-r_rotation.pitch), Math::radians(-r_rotation.yaw));
        Math::mat4 modelViewInverseMatrix = scaleInverseMatrix * rotationInverseMatrix * translationInverseMatrix;

        auto par = parent.lock();
        if (par) {
            auto par3d = std::dynamic_pointer_cast<Node3D>(par);
            if (par3d) {
                return modelViewInverseMatrix * par3d->getInverseModelViewMatrix();
            }
        }

        return modelViewInverseMatrix;
    }

};


}

#endif//OJOIE_3DNODE_HPP
