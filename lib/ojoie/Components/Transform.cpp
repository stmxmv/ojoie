//
// Created by aojoie on 4/2/2023.
//

#include "Components/Transform.hpp"
#include "Core/Actor.hpp"
#include "Utility/Log.h"

#include <imgui.h>

namespace AN {

const Name kDidChangeTransformMessage = "DidChangeTransform";

using namespace Math;

IMPLEMENT_AN_CLASS(Transform)
LOAD_AN_CLASS(Transform)
IMPLEMENT_AN_OBJECT_SERIALIZE(Transform)
INSTANTIATE_TEMPLATE_TRANSFER(Transform)

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

Transform::Transform(ObjectCreationMode mode)
    : Super(mode), _parent(),
      _localRotation(), _localScale(), _localPosition(),
      _cachedTransformMatrix(), _hasCachedTransformMatrix(), _hasChanged(true) {}

bool Transform::init() {
    if (!Super::init()) return false;
    _parent                = nullptr;
    _localPosition         = {};
    _localRotation         = Math::identity<Quaternionf>();
    _localScale            = { 1.f, 1.f, 1.f };
    _cachedTransformMatrix = {};
    setCacheDirty();
    return true;
};


Transform::~Transform() {}

void Transform::setCacheDirty() {
    _hasCachedTransformMatrix = false;
    _hasChanged               = true;

    TransformComList::iterator end = children.end();
    for (TransformComList::iterator i = children.begin(); i != end; ++i) {
        (*i)->setCacheDirty();
    }
}


Vector3f Transform::getPosition() const {
    Vector3f            worldPos = _localPosition;
    Transform          *cur      = getParent();
    while (cur) {

        worldPos *= cur->_localScale;
        worldPos = rotate(cur->_localRotation, worldPos);
        worldPos += cur->_localPosition;

        cur = cur->getParent();
    }

    return worldPos;
}

Matrix3x3f Transform::GetWorldRotationAndScale() const {
    Matrix3x3f scale    = Math::scale(_localScale);
    Matrix3x3f rotation = Math::toMat3(_localRotation);

    Transform *parent = getParent();
    if (parent) {
        Matrix3x3f parentTransform = parent->GetWorldRotationAndScale();
        return parentTransform * rotation * scale;
    }
    return rotation * scale;
}

void Transform::SetWorldRotationAndScale(const Matrix3x3f &scale)
{
    _localScale = Vector3f(1.f);

    Matrix3x3f inverseRS = GetWorldRotationAndScale();
    inverseRS = Math::inverse(inverseRS);

    inverseRS = inverseRS * scale;

    _localScale.x = inverseRS[0][0];
    _localScale.y = inverseRS[1][1];
    _localScale.z = inverseRS[2][2];

    setCacheDirty();
    sendTransformMessage(kRotationChanged | kScaleChanged | kPositionChanged);
}

Matrix3x3f Transform::GetWorldScale() const
{
    Quaternionf invRotation(Math::inverse(getRotation()));
    Matrix3x3f scaleAndRotation = GetWorldRotationAndScale();
    return Math::toMat3(invRotation) * scaleAndRotation;
}

Quaternionf Transform::getRotation() const {
    Quaternionf         worldRot = _localRotation;
    Transform          *cur      = getParent();
    while (cur) {
        worldRot = cur->_localRotation * worldRot;
        cur      = cur->getParent();
    }
    return worldRot;
}

Vector3f Transform::GetLocalEulerAngles() const
{
    Quaternionf qu = _localRotation;
    float yaw, pitch, roll;
    Math::extractEulerAngleYXZ(Math::toMat4(qu), yaw, pitch, roll);
    return { Math::degrees(pitch), Math::degrees(yaw), Math::degrees(roll) };
}

Vector3f Transform::getEulerAngles() const {
    Quaternionf qu = getRotation();
    float yaw, pitch, roll;
    Math::extractEulerAngleYXZ(Math::toMat4(qu), yaw, pitch, roll);
    return { Math::degrees(pitch), Math::degrees(yaw), Math::degrees(roll) };
}

void Transform::setLocalEulerAngles(const Vector3f &angles)
{
    setLocalRotation(Math::toQuat(Math::eulerAngleYXZ(Math::radians(angles.y), Math::radians(angles.x), Math::radians(angles.z))));
}

void Transform::setEulerAngles(const Vector3f &angles) {
    setRotation(Math::toQuat(Math::eulerAngleYXZ(Math::radians(angles.y), Math::radians(angles.x), Math::radians(angles.z))));
    sendTransformMessage(kRotationChanged);
}

void Transform::setLocalRotation(const Quaternionf &localRotation) {
    _localRotation = localRotation;
    setCacheDirty();
    sendTransformMessage(kRotationChanged);
}

void Transform::setLocalScale(const Vector3f &localScale) {
    _localScale = localScale;
    setCacheDirty();
    sendTransformMessage(kScaleChanged);
}

void Transform::setLocalPosition(const Vector3f &localPosition) {
    _localPosition = localPosition;
    setCacheDirty();
    sendTransformMessage(kPositionChanged);
}

void Transform::setRotation(const Quaternionf &q) {
    ABORT_INVALID_QUATERNION(q, rotation, transform);

    Transform *father = getParent();
    if (father != nullptr)
        setLocalRotation(Math::normalize(Math::inverse(father->getRotation()) * q));
    else
        setLocalRotation(Math::normalize(q));

    setCacheDirty();
}

void Transform::setPosition(const Vector3f &p) {
    ABORT_INVALID_VECTOR3(p, position, transform);

    Vector3f            newPosition = p;
    Transform          *father      = getParent();
    if (father != nullptr)
        newPosition = father->inverseTransformPoint(newPosition);

    setLocalPosition(newPosition);

    setCacheDirty();
}

void Transform::setWorldRotationAndScale(const Matrix3x3f &worldRotationAndScale) {
    _localScale = Vector3f(1.f);

    Matrix3x3f inverseRS = GetWorldRotationAndScale();
    inverseRS            = Math::inverse(inverseRS);

    inverseRS = inverseRS * worldRotationAndScale;

    _localScale.x = inverseRS[0][0];
    _localScale.y = inverseRS[1][1];
    _localScale.z = inverseRS[2][2];

    setCacheDirty();
    sendTransformMessage(kScaleChanged | kRotationChanged | kPositionChanged);
}

Vector3f Transform::inverseTransformPoint(const Vector3f &inPosition) {
    Vector3f            newPosition, localPosition;
    Transform          *father = getParent();
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

Matrix4x4f Transform::getWorldToLocalMatrix() const {
    Matrix4x4f m;
    m = Math::scale(1.f / _localScale) *
        Math::toMat4(Math::inverse(_localRotation)) *
        Math::translate(-_localPosition);

    Transform * father = getParent();
    if (father != nullptr) {
        Matrix4x4f parentMat = father->getWorldToLocalMatrix();
        m = m * parentMat;
    }

    return m;
}

Matrix4x4f Transform::GetWorldToLocalMatrixNoScale() const
{
    Quaternionf rot; Vector3f pos;
    GetPositionAndRotation(pos, rot);
    Matrix4x4f m = Math::toMat4(Math::inverse(rot)) * Math::translate(-pos);
    return m;
}

void Transform::GetPositionAndRotation(Vector3f &position, Quaternionf &rotation) const
{
    Vector3f worldPos = _localPosition;
    Quaternionf worldRot = _localRotation;
    Transform* cur = getParent();
    while (cur)
    {
        worldPos *= cur->_localScale;
        worldPos = cur->_localRotation * worldPos;
        worldPos += cur->_localPosition;
        worldRot = cur->_localRotation * worldRot;
        cur = cur->getParent();
    }

    position = worldPos;
    rotation = worldRot;
}

Matrix4x4f Transform::GetLocalToWorldMatrixNoScale() const
{
    Quaternionf rot; Vector3f pos;
    GetPositionAndRotation(pos, rot);
    Matrix4x4f m = Math::translate(pos) * Math::toMat4(rot);
    return m;
}

Matrix4x4f Transform::getLocalToWorldMatrix() const {
    Matrix4x4f m;
    calculateTransformMatrix(m);
    return m;
}

void Transform::calculateLocalTransformMatrix(Matrix4x4f &matrix) const {
    matrix = Math::translate(_localPosition) * Math::toMat4(_localRotation) * Math::scale(_localScale);
}

void Transform::calculateTransformMatrixIterative(Matrix4x4f &transform) const {
    if (_hasCachedTransformMatrix) {
        transform = _cachedTransformMatrix;
        return;
    }

    calculateLocalTransformMatrix(transform);

    Transform          *parent = getParent();
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

void Transform::calculateTransformMatrix(Matrix4x4f &matrix) const {
    if (_hasCachedTransformMatrix) {
        matrix = _cachedTransformMatrix;
        return;
    }

    const Transform          *transforms[32];
    int                       transformCount = 1;
    Matrix4x4f                temp;

    {
        transforms[0]              = this;
        Transform *parent = nullptr;
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
        const Transform *t      = transforms[i];
        const Transform *parent = transforms[i + 1];
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

void Transform::setParent(Transform *newParent, bool worldPositionStays) {
    if (getActor().isDestroying() || (newParent && newParent->getActor().isDestroying())) {
        return;
    }

    if (getActor().isActivating() || (newParent && newParent->getActor().isActivating())) {
        ANLog("Cannot change Actor hierarchy while activating or deactivating the parent");
        return;
    }

    Transform *cur;
    for (cur = newParent; cur != nullptr; cur = cur->_parent) {
        if (this == cur)
            return;
    }

    Transform *father = getParent();
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

        Vector3f    worldPosition = getPosition();
        Quaternionf worldRotation = getRotation();
        Matrix3x3f  worldScale    = GetWorldRotationAndScale();

        setRotation(worldRotation);
        setPosition(worldPosition);
        setWorldRotationAndScale(worldScale);

        sendTransformMessage(kParentingChanged);
    } else {
        sendTransformMessage(kPositionChanged | kRotationChanged | kScaleChanged | kParentingChanged);
    }
    setCacheDirty();
}

void Transform::moveChildUp(Transform *child) {
    auto iter = std::find(children.begin(), children.end(), child);
    if (iter != children.end() && iter != children.begin()) {
        std::iter_swap(iter, iter - 1);
    }
}

void Transform::moveChildDown(Transform *child) {
    auto iter = std::find(children.begin(), children.end(), child);
    if (iter != children.end() && iter != children.end() - 1) {
        std::iter_swap(iter, iter + 1);
    }
}

bool revertButton(void *id) {
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    bool ret = ImGui::Button("Revert");
    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return ret;
}

void Transform::sendTransformMessage(TransformChangeFlags flags) {

    for (Transform *child : children)
    {
        child->sendTransformMessage(flags);
    }

    _hasCachedTransformMatrix = false;
    _hasChanged               = true;

    Message message;
    message.sender = this;
    message.name = kDidChangeTransformMessage;
    message.data = flags;
    getActor().sendMessage(message);
}

Transform *Transform::find(const char *name)
{
    if (getActor().getName() == name)
    {
        return this;
    }

    for (Transform *child : children)
    {
        if (Transform *trans = child->find(name); trans != nullptr)
        {
            return trans;
        }
    }

    return nullptr;
}

void Transform::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    static const char *position_str = "P";

    ImGui::Text("%s", position_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(position_str).x) * 0.25f );

    Vector3f position = _localPosition;
    Vector3f scale = _localScale;
    Vector3f rotation = GetLocalEulerAngles();

    if (ImGui::DragFloat("X##position", &position.x, 0.01f, 0, 0, "%.5g")) {
        setLocalPosition(position);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y##position", &position.y, 0.01f, 0, 0, "%.5g")) {
        setLocalPosition(position);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Z##position", &position.z, 0.01f, 0, 0, "%.5g")) {
        setLocalPosition(position);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (revertButton(&_localPosition)) {
        position = {};
        setLocalPosition(position);
    }
    ImGui::PopItemWidth();
    //            ImGui::EndChild();


    static const char *rotation_str = "R";
    ImGui::Text("%s", rotation_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(rotation_str).x) * 0.25f );
    if (ImGui::DragFloat("P", &rotation.x, 0.01f, 0, 0, "%.5g")) {
        Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
        setLocalRotation(Math::toQuat(mat));
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y", &rotation.y, 0.01f, 0, 0, "%.5g")) {
        Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
        setLocalRotation(Math::toQuat(mat));
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("R", &rotation.z, 0.01f, 0, 0, "%.5g")) {
        Matrix4x4f mat = Math::eulerAngleYXZ(Math::radians(rotation.y), Math::radians(rotation.x), Math::radians(rotation.z));
        setLocalRotation(Math::toQuat(mat));
    }
    ImGui::SameLine();
    if (revertButton(&_localRotation)) {
       setLocalRotation(Math::identity<Quaternionf>());
    }
    ImGui::PopItemWidth();



    static const char *scale_str = "S";
    ImGui::Text("%s", scale_str);
    ImGui::SameLine();
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(scale_str).x) * 0.25f );
    if (ImGui::DragFloat("X##scale", &scale.x, 0.01f, 0, 0, "%.5g")) {
        setLocalScale(scale);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Y##scale", &scale.y, 0.01f, 0, 0, "%.5g")) {
        setLocalScale(scale);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Z##scale", &scale.z, 0.01f, 0, 0, "%.5g")) {
        setLocalScale(scale);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
    }
    ImGui::SameLine();
    if (revertButton(&_localScale)) {
        scale = { 1.f, 1.f, 1.f };
        setLocalScale(scale);
    }

    ImGui::PopItemWidth();
#endif
}

template<typename _Coder>
void Transform::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(children);
    TRANSFER(_parent);
    TRANSFER(_localRotation);
    TRANSFER(_localScale);
    TRANSFER(_localPosition);
}

}// namespace AN