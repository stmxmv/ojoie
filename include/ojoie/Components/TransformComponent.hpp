//
// Created by aojoie on 4/2/2023.
//

#ifndef OJOIE_TRANSFORM_HPP
#define OJOIE_TRANSFORM_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Math/Math.hpp>
#include <vector>

namespace AN {

class AN_API TransformComponent : public Component {

    DECLARE_DERIVED_AN_CLASS(TransformComponent, Component);

    typedef std::vector<TransformComponent *> TransformComList;

private:
    TransformComList    children;
    TransformComponent *_parent;

    Quaternionf _localRotation;
    Vector3f    _localScale;
    Vector3f    _localPosition;

    mutable Matrix4x4f _cachedTransformMatrix;
    mutable bool       _hasCachedTransformMatrix;
    mutable bool       _hasChanged;

    void setCacheDirty();

    void calculateTransformMatrixIterative(Matrix4x4f& matrix) const;

    void calculateLocalTransformMatrix(Matrix4x4f& matrix) const;

public:
    explicit TransformComponent(ObjectCreationMode mode);

    static bool IsSealedClass() { return true; }

    virtual bool init() override;

    void setParent(TransformComponent *parent, bool worldPositionStays);

    TransformComponent *getParent() const { return _parent; }
    const TransformComList &getChildren() const { return children; }

    Quaternionf getLocalRotation() const { return _localRotation; }
    Vector3f getLocalScale() const { return _localScale; }
    Vector3f getLocalPosition() const { return _localPosition; }

    /// Gets the transform from local to world space
    Vector3f getPosition() const;
    /// Returns the world rotation and scale.
    /// (It is impossible to return a Vector3 because the scale might be skewed)
    Matrix3x3f getWorldRotationAndScale() const;
    Quaternionf getRotation() const;
    Vector3f getEulerAngles() const;

    void setLocalRotation(const Quaternionf &localRotation);
    void setLocalScale(const Vector3f &localScale);
    void setLocalPosition(const Vector3f &localPosition);

    void setRotation(const Quaternionf &localRotation);
    void setPosition(const Vector3f &localPosition);
    void setWorldRotationAndScale(const Matrix3x3f &worldRotationAndScale);

    Vector3f inverseTransformPoint(const Vector3f &inPosition);

    /// Return matrix that converts a point from World To Local space
    Matrix4x4f getWorldToLocalMatrix() const;
    /// Return matrix that converts a point from Local To World space
    Matrix4x4f getLocalToWorldMatrix() const;

    void calculateTransformMatrix(Matrix4x4f& matrix) const;
};

}// namespace AN

#endif//OJOIE_TRANSFORM_HPP
