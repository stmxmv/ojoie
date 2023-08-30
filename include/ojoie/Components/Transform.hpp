//
// Created by aojoie on 4/2/2023.
//

#ifndef OJOIE_TRANSFORM_HPP
#define OJOIE_TRANSFORM_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Math/Math.hpp>
#include <vector>

namespace AN {

enum TransformChangeFlagBits {
    kPositionChanged = 1 << 0,
    kRotationChanged = 1 << 1,
    kScaleChanged = 1 << 3,
    kAnimatePhysics = 1 << 4,
    kParentingChanged = 1 << 5
};

typedef ANFlags TransformChangeFlags;

AN_API extern const Name kDidChangeTransformMessage; /// data is TransformChangeFlag

class AN_API Transform : public Component {

    DECLARE_DERIVED_AN_CLASS(Transform, Component);

    typedef std::vector<Transform *> TransformComList;

private:
    TransformComList    children;
    Transform          *_parent;

    Quaternionf _localRotation;
    Vector3f    _localScale;
    Vector3f    _localPosition;

    mutable Matrix4x4f _cachedTransformMatrix;
    mutable bool       _hasCachedTransformMatrix;
    mutable bool       _hasChanged;

    void setCacheDirty();

    void calculateTransformMatrixIterative(Matrix4x4f& matrix) const;

    void calculateLocalTransformMatrix(Matrix4x4f& matrix) const;

    void sendTransformMessage(TransformChangeFlags flags);

public:
    explicit Transform(ObjectCreationMode mode);

    static bool IsSealedClass() { return true; }

    virtual bool init() override;

    void setParent(Transform *parent, bool worldPositionStays);

    Transform              *getParent() const { return _parent; }
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
    void setEulerAngles(const Vector3f &angles);

    Vector3f inverseTransformPoint(const Vector3f &inPosition);

    /// Return matrix that converts a point from World To Local space
    Matrix4x4f getWorldToLocalMatrix() const;
    /// Return matrix that converts a point from Local To World space
    Matrix4x4f getLocalToWorldMatrix() const;

    void calculateTransformMatrix(Matrix4x4f& matrix) const;

    /// editor support
    void moveChildUp(Transform *child);
    void moveChildDown(Transform *child);

    virtual void onInspectorGUI() override;
};

}// namespace AN

#endif//OJOIE_TRANSFORM_HPP
