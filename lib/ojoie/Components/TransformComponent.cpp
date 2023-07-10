//
// Created by aojoie on 4/2/2023.
//

#include "Components/TransformComponent.hpp"
#include "Core/Actor.hpp"
#include "Utility/Log.h"

namespace AN {

using namespace Math;

IMPLEMENT_AN_CLASS(TransformComponent);
LOAD_AN_CLASS(TransformComponent);

#define ABORT_INVALID_QUATERNION(value, varname, classname)                                                                                                                                                                                                                           \
    if (!isFinite(value)) {                                                                                                                                                                                                                                                           \
        AN_LOG(Error, "%s.%s assign attempt for '%s' is not valid. Input rotation is { %s, %s, %s, %s }.", #classname, #varname, getClassName(), std::to_string(value.x).c_str(), std::to_string(value.y).c_str(), std::to_string(value.z).c_str(), std::to_string(value.w).c_str()); \
        return;                                                                                                                                                                                                                                                                       \
    }

#define ABORT_INVALID_VECTOR3(value, varname, classname)                                                                                                                                                                                             \
    if (!isFinite(value)) {                                                                                                                                                                                                                          \
        AN_LOG(Error, "%s.%s assign attempt for '%s' is not valid. Input %s is { %s, %s, %s }.", #classname, #varname, getClassName(), #varname, std::to_string(value.x).c_str(), std::to_string(value.y).c_str(), std::to_string(value.z).c_str()); \
        return;                                                                                                                                                                                                                                      \
    }

TransformComponent::TransformComponent(ObjectCreationMode mode)
    : Super(mode), _parent(),
      _localRotation(), _localScale(), _localPosition(),
      _cachedTransformMatrix(), _hasCachedTransformMatrix(), _hasChanged(true) {}

bool TransformComponent::init() {
    if (!Super::init()) return false;
    _parent                = nullptr;
    _localPosition         = {};
    _localRotation         = Math::identity<Quaternionf>();
    _localScale            = { 1.f, 1.f, 1.f };
    _cachedTransformMatrix = {};
    setCacheDirty();
    return true;
};


TransformComponent::~TransformComponent() {}

void TransformComponent::setCacheDirty() {
    _hasCachedTransformMatrix = false;
    _hasChanged               = true;

    TransformComList::iterator end = children.end();
    for (TransformComList::iterator i = children.begin(); i != end; ++i) {
        (*i)->setCacheDirty();
    }
}


Vector3f TransformComponent::getPosition() const {
    Vector3f            worldPos = _localPosition;
    TransformComponent *cur      = getParent();
    while (cur) {

        worldPos *= cur->_localScale;
        worldPos = rotate(cur->_localRotation, worldPos);
        worldPos += cur->_localPosition;

        cur = cur->getParent();
    }

    return worldPos;
}

Matrix3x3f TransformComponent::getWorldRotationAndScale() const {
    Matrix3x3f scale    = Math::scale(_localScale);
    Matrix3x3f rotation = Math::toMat3(_localRotation);

    TransformComponent *parent = getParent();
    if (parent) {
        Matrix3x3f parentTransform = parent->getWorldRotationAndScale();
        return parentTransform * rotation * scale;
    }
    return rotation * scale;
}

Quaternionf TransformComponent::getRotation() const {
    Quaternionf         worldRot = _localRotation;
    TransformComponent *cur      = getParent();
    while (cur) {
        worldRot = cur->_localRotation * worldRot;
        cur      = cur->getParent();
    }
    return worldRot;
}

Vector3f TransformComponent::getEulerAngles() const {
    Quaternionf qu = getRotation();
    float yaw, pitch, roll;
    Math::extractEulerAngleYXZ(Math::toMat4(qu), yaw, pitch, roll);
    return { Math::degrees(pitch), Math::degrees(yaw), Math::degrees(roll) };
}

void TransformComponent::setLocalRotation(const Quaternionf &localRotation) {
    _localRotation = localRotation;
    setCacheDirty();
}

void TransformComponent::setLocalScale(const Vector3f &localScale) {
    _localScale = localScale;
    setCacheDirty();
}

void TransformComponent::setLocalPosition(const Vector3f &localPosition) {
    _localPosition = localPosition;
    setCacheDirty();
}

void TransformComponent::setRotation(const Quaternionf &q) {
    ABORT_INVALID_QUATERNION(q, rotation, transform);

    TransformComponent *father = getParent();
    if (father != nullptr)
        setLocalRotation(Math::normalize(Math::inverse(father->getRotation()) * q));
    else
        setLocalRotation(Math::normalize(q));

    setCacheDirty();
}

void TransformComponent::setPosition(const Vector3f &p) {
    ABORT_INVALID_VECTOR3(p, position, transform);

    Vector3f            newPosition = p;
    TransformComponent *father      = getParent();
    if (father != nullptr)
        newPosition = father->inverseTransformPoint(newPosition);

    setLocalPosition(newPosition);

    setCacheDirty();
}

void TransformComponent::setWorldRotationAndScale(const Matrix3x3f &worldRotationAndScale) {
    _localScale = Vector3f(1.f);

    Matrix3x3f inverseRS = getWorldRotationAndScale();
    inverseRS            = Math::inverse(inverseRS);

    inverseRS = inverseRS * worldRotationAndScale;

    _localScale.x = inverseRS[0][0];
    _localScale.y = inverseRS[1][1];
    _localScale.z = inverseRS[2][2];

    setCacheDirty();
}

Vector3f TransformComponent::inverseTransformPoint(const Vector3f &inPosition) {
    Vector3f            newPosition, localPosition;
    TransformComponent *father = getParent();
    if (father)
        localPosition = father->inverseTransformPoint(inPosition);
    else
        localPosition = inPosition;

    localPosition -= _localPosition;
    newPosition = Math::rotate(Math::inverse(_localRotation), localPosition);
    //    if (m_InternalTransformType != kNoScaleTransform)
    newPosition /= _localScale;

    return newPosition;
}

Matrix4x4f TransformComponent::getWorldToLocalMatrix() const {
    Matrix4x4f m;
    m = Math::scale(_localScale / 1.f) *
        Math::toMat4(Math::inverse(_localRotation)) *
        Math::translate(-_localPosition);

    TransformComponent* father = getParent();
    if (father != nullptr) {
        Matrix4x4f parentMat = father->getWorldToLocalMatrix();
        m = m * parentMat;
    }

    return m;
}

Matrix4x4f TransformComponent::getLocalToWorldMatrix() const {
    Matrix4x4f m;
    calculateTransformMatrix(m);
    return m;
}

void TransformComponent::calculateLocalTransformMatrix(Matrix4x4f &matrix) const {
    matrix = Math::translate(_localPosition) * Math::toMat4(_localRotation) * Math::scale(_localScale);
}

void TransformComponent::calculateTransformMatrixIterative(Matrix4x4f &transform) const {
    if (_hasCachedTransformMatrix) {
        transform = _cachedTransformMatrix;
        return;
    }

    calculateLocalTransformMatrix(transform);

    TransformComponent *parent = getParent();
    Matrix4x4f          temp;
    while (parent != nullptr) {
        if (parent->_hasCachedTransformMatrix) {
            temp = parent->_cachedTransformMatrix * transform;
            parent = nullptr;
        } else {
            Matrix4x4f parentTransform;
            parent->calculateLocalTransformMatrix(parentTransform);
            temp   = parentTransform * transform;
            parent = parent->getParent();
        }
        transform = temp;
    }

    _cachedTransformMatrix    = transform;
    _hasCachedTransformMatrix = true;
}

void TransformComponent::calculateTransformMatrix(Matrix4x4f &matrix) const {
    if (_hasCachedTransformMatrix) {
        matrix = _cachedTransformMatrix;
        return;
    }

    const TransformComponent *transforms[32];
    int                       transformCount = 1;
    Matrix4x4f                temp;

    {
        transforms[0]              = this;
        TransformComponent *parent = nullptr;
        for (parent = getParent(); parent != nullptr && !parent->_hasCachedTransformMatrix; parent = parent->getParent()) {
            transforms[transformCount++] = parent;
            // reached maximum of transforms that we can calculate - fallback to old method
            if (transformCount == 31) {
                parent = parent->getParent();
                if (parent) {
                    parent->calculateTransformMatrixIterative(temp);
                    ANAssert(parent->_hasCachedTransformMatrix);
                }
                break;
            }
        }

        transforms[transformCount] = parent;
        ANAssert(transformCount <= 31);
    }

    for (int i = transformCount - 1; i >= 0; --i) {
        const TransformComponent *t      = transforms[i];
        const TransformComponent *parent = transforms[i + 1];
        if (parent) {
            ANAssert(parent->_hasCachedTransformMatrix);
            t->calculateLocalTransformMatrix(temp);
            //            type |= (TransformType)parent->m_CachedTransformType;
            t->_cachedTransformMatrix = parent->_cachedTransformMatrix * temp;
        } else {
            // Build the local transform into m_CachedTransformMatrix
            t->calculateLocalTransformMatrix(t->_cachedTransformMatrix);
        }
        // store cached transform
        //        t->m_CachedTransformType = UpdateTransformType(type, t);
        t->_hasCachedTransformMatrix = true;
    }

    ANAssert(_hasCachedTransformMatrix);
    matrix = _cachedTransformMatrix;
}

void TransformComponent::setParent(TransformComponent *newParent, bool worldPositionStays) {
    if (getActor().isDestroying() || (newParent && newParent->getActor().isDestroying())) {
        return;
    }

    if (getActor().isActivating() || (newParent && newParent->getActor().isActivating())) {
        ANLog("Cannot change Actor hierarchy while activating or deactivating the parent");
        return;
    }

    TransformComponent *cur;
    for (cur = newParent; cur != nullptr; cur = cur->_parent) {
        if (this == cur)
            return;
    }

    Vector3f    worldPosition = getPosition();
    Quaternionf worldRotation = getRotation();
    Matrix3x3f  worldScale    = getWorldRotationAndScale();

    TransformComponent *father = getParent();
    if (father != nullptr) {
        auto iter = std::find(father->children.begin(), father->children.end(), this);
        ANAssert(iter != father->children.end());
        father->children.erase(iter);
    }

    if (newParent) {
        newParent->children.push_back(this);
    }

    _parent = newParent;

    if (worldPositionStays) {
        setRotation(worldRotation);
        setPosition(worldPosition);
        setWorldRotationAndScale(worldScale);
    }
    setCacheDirty();
}

}// namespace AN