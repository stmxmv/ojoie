//
// Created by Aleudillonam on 8/3/2023.
//

#pragma once

#include <ojoie/Core/Component.hpp>

namespace AN {

class RigidBody;

class Collider : public Component {

    struct Impl;
    Impl *impl;

    static void OnTransformChangeMessage(void *receiver, Message &message);
    static void OnAddComponentMessage(void *receiver, Message &message);
    static void OnRemoveRigidBodyMessage(void *receiver, Message &message);

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Collider, Component)

protected:

    virtual void create(RigidBody *ignored) = 0;
    void finishCreate(void *shapeDesc, RigidBody *ignored);
    void cleanup();

    void *getShape();

public:

    static void InitializeClass();

    explicit Collider(ObjectCreationMode mode);

    virtual bool init() override;

    virtual void dealloc() override;

#ifdef OJOIE_WITH_EDITOR
    bool m_Visualization = false;
#endif
    virtual void onInspectorGUI() override;
};


}
