//
// Created by Aleudillonam on 8/3/2023.
//

#pragma once

#include <ojoie/Core/Component.hpp>

namespace AN {

AN_API extern const Name kWillRemoveRigidBodyMessage;

class RigidBody : public Component {

    struct Impl;
    Impl *impl;

    bool m_AddToScene;
    float m_Mass;
    float m_Drag;

    bool m_UseGravity;

    static void OnTransformChangedMessage(void *receiver, Message &message);

    DECLARE_DERIVED_AN_CLASS(RigidBody, Component)

public:

    static void InitializeClass();

    explicit RigidBody(ObjectCreationMode mode);

    virtual void dealloc() override;

    virtual bool init() override;

    virtual void onInspectorGUI() override;

    void *getUnderlyingObject();
};

}
