//
// Created by Aleudillonam on 8/4/2023.
//

#pragma once

#include <ojoie/Physics/Collider.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>

namespace AN {

class RigidBody;
class MeshCollider : public Collider {

    Mesh *m_Mesh;

    DECLARE_DERIVED_AN_CLASS(MeshCollider, Collider)
protected:

    virtual void create(RigidBody *ignored) override;

public:

    explicit MeshCollider(ObjectCreationMode mode);

    virtual void onInspectorGUI() override;
};


}
