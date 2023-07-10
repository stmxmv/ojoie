//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_3DNODE_HPP
#define OJOIE_3DNODE_HPP

#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/Core/Node.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Math/Transform.hpp>

namespace AN {


/// \brief a node3d has a position in the 3d world
class Node3D : public Node {
    typedef Node3D Self;
    typedef Node Super;

    Math::Transform transform;

    bool didSetTransform{ true };

public:

    struct Node3DSceneProxy : Super::SceneProxyType {
        Math::Transform transform;

        explicit Node3DSceneProxy(Node3D &node) : Super::SceneProxyType(node) {
            transform = node.transform;
        }

        Math::mat4 getModelViewMatrix() {
            Math::mat4 modelMatrix = transform.toMatrix();

            auto par = owner.parent();
            if (par) {
                auto par3d = std::dynamic_pointer_cast<Node3D>(par);
                if (par3d) {
                    auto *parentProxy = par3d->sceneProxy;
                    return ((Node3DSceneProxy *)parentProxy)->getModelViewMatrix()
                           * modelMatrix;
                }
            }
            return modelMatrix;
        }

        Math::mat4 getInverseModelViewMatrix() {
            Math::mat4 inverseModelMatrix = transform.toInverseMatrix();

            auto par = owner.parent();
            if (par) {
                auto par3d = std::dynamic_pointer_cast<Node3D>(par);
                if (par3d) {
                    auto *parentProxy = par3d->sceneProxy;
                    return inverseModelMatrix * ((Node3DSceneProxy *)parentProxy)->getInverseModelViewMatrix();
                }
            }

            return inverseModelMatrix;
        }

    };

    typedef Node3DSceneProxy SceneProxyType;

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual RC::SceneProxy *createSceneProxy() override {
        return new Node3DSceneProxy(*this);
    }

    virtual void updateSceneProxy() override {
        Super::updateSceneProxy();
        if (didSetTransform) {
            sceneProxy->retain();
            Dispatch::async(Dispatch::Render, [trans = transform, sceneProxy = sceneProxy] {
                auto *proxy = (Node3DSceneProxy *)(sceneProxy);
                proxy->transform = trans;
                proxy->release();
            });

            didSetTransform = false;
        }
    }

    const Math::vec3 &getPosition() const {
        return transform.translation;
    }

    const Math::vec3 &getScale() const {
        return transform.scale;
    }

    Math::rotator getRotation() const {
        return transform.rotator();
    }

    void setPosition(const Math::vec3 &position) {
        transform.translation = position;
        didSetTransform = true;
    }

    void setScale(const Math::vec3 &scale) {
        transform.scale = scale;
        didSetTransform = true;
    }

    void setRotation(const Math::rotator &rotation) {
        Math::qua<float> qua(Math::vec3(0.f));

        qua = Math::angleAxis(Math::radians(rotation.yaw), Math::vec3{ 0.f, 1.f, 0.f }) *
              Math::angleAxis(Math::radians(rotation.pitch), Math::vec3{ 1.f, 0.f, 0.f }) *
              Math::angleAxis(Math::radians(rotation.roll), Math::vec3{ 0.f, 0.f, 1.f });

        transform.rotation = qua;
        didSetTransform = true;
    }

    /////////////////////////////// render's method

    static std::shared_ptr<class CameraNode> GetCurrentCamera();

    static void SetCurrentCamera(const std::shared_ptr<class CameraNode> &cameraNode);



};


}

#endif//OJOIE_3DNODE_HPP
