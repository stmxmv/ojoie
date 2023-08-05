//
// Created by Aleudillonam on 8/3/2023.
//

#pragma once

#include <ojoie/Physics/Collider.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {

class RigidBody;
class BoxCollider : public Collider {

    Vector3f m_Size;

    DECLARE_DERIVED_AN_CLASS(BoxCollider, Collider)

protected:

    virtual void create(RigidBody *ignored) override;

public:

    explicit BoxCollider(ObjectCreationMode mode);

    void setSize(const Vector3f &size);

    virtual void onInspectorGUI() override;
};


}
