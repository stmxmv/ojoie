//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_NODE2D_HPP
#define OJOIE_NODE2D_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {


class Node2D : public Node {
    typedef Node2D Self;
    typedef Node Super;


    Math::vec2 _position{};
    bool didSetPosition{};

public:

    struct Node2DSceneProxy : Super::SceneProxyType {
        Math::vec2 position;

        explicit Node2DSceneProxy(Node2D &node) : Super::SceneProxyType(node) {
            position = node._position;
        }

    };

    typedef Node2DSceneProxy SceneProxyType;

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


    virtual RC::SceneProxy *createSceneProxy() override {
        return new Node2DSceneProxy(*this);
    }

    virtual void updateSceneProxy() override {
        Super::updateSceneProxy();

        if (didSetPosition) {
            sceneProxy->retain();

            Dispatch::async(Dispatch::Render, [position = _position, sceneProxy = sceneProxy] {
                auto *proxy = (Node2DSceneProxy *)sceneProxy;
                proxy->position = position;

                proxy->release();
            });

            didSetPosition = false;
        }

    }

    const Math::vec2 &getPosition() const {
        return _position;
    }

    void setPosition(const Math::vec2 &position) {
        _position = position;
        didSetPosition = true;
    }

};

}

#endif//OJOIE_NODE2D_HPP
