//
// Created by aojoie on 10/18/2022.
//

#ifndef OJOIE_MATH_TRANSFORM_HPP
#define OJOIE_MATH_TRANSFORM_HPP

#include <ojoie/Math/Math.hpp>

namespace AN::Math {

struct Transform {
    Math::qua<float> rotation;
    Math::vec3 scale;
    Math::vec3 translation;

    Transform() : rotation(Math::identity<Math::qua<float>>()), scale(1.f), translation() {}

    rotator rotator() const {
        float yaw, pitch, roll;
        Math::extractEulerAngleYXZ(Math::toMat4(rotation), yaw, pitch, roll);
        return { .pitch = Math::degrees(pitch), .roll = Math::degrees(roll), .yaw = Math::degrees(yaw) };
    }

    Math::mat4 toMatrix() const {
        Math::mat4 translateMatrix = Math::translate(Math::identity<Math::mat4>(), translation);
        Math::mat4 rotationScaleMatrix = Math::toMat4(rotation) * Math::scale(Math::identity<Math::mat4>(), scale);
        return translateMatrix * rotationScaleMatrix;
    }

    Math::mat4 toInverseMatrix() const {
        Math::mat4 inverseScaleMatrix = Math::scale(Math::identity<Math::mat4>(), 1.f / scale);
        Math::mat4 inverseRotationTranslateMatrix = Math::toMat4(Math::inverse(rotation))
                                         * Math::translate(Math::identity<Math::mat4>(), -translation);
        return inverseScaleMatrix * inverseRotationTranslateMatrix;
    }

};

}

#endif//OJOIE_MATH_TRANSFORM_HPP
