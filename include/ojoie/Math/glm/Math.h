//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_MATH_H
#define OJOIE_MATH_H

#include "ojoie/Configuration/typedef.h"

#define GLM_FORCE_RADIANS

#if !defined(OJOIE_USE_OPENGL)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace AN {

namespace Math {

using namespace glm;

template <typename T>
requires (!std::ranges::range<T>)
inline void hash_combine(size_t &seed, T &&v) {
    glm::detail::hash_combine(seed, std::hash<std::decay_t<T>>{}(v));
}

template<std::ranges::range _Rng>
inline void hash_combine(size_t &seed, _Rng rng) {
    glm::detail::hash_combine(seed, std::hash<_Rng>{}(rng));
}

template<>
inline void hash_combine(size_t &seed, bool &v0) {
    glm::detail::hash_combine(seed, std::hash<UInt32>{}((UInt32)v0));
}

template<>
inline void hash_combine(size_t &seed, bool &&v0) {
    glm::detail::hash_combine(seed, std::hash<UInt32>{}((UInt32)v0));
}

inline bool isFinite(const float& value) {
    // Returns false if value is NaN or +/- infinity
    uint32_t intval = *reinterpret_cast<const uint32_t*>(&value);
    return (intval & 0x7f800000) != 0x7f800000;
}

inline bool isFinite(const Math::qua<float> &q) {
    return isFinite(q.x) && isFinite(q.y) && isFinite(q.z) && isFinite(q.w);
}

}

typedef Math::vec2 Vector2f;
typedef Math::vec3 Vector3f;
typedef Math::vec4 Vector4f;
typedef Math::mat4 Matrix4x4f;
typedef Math::mat3 Matrix3x3f;
typedef Math::qua<float> Quaternionf;


template<>
struct SerializeTraits<Vector4f> : public SerializeTraitsBase<Vector4f> {

    typedef Vector4f value_type;

    inline static const char* GetTypeString () { return "Vector4f"; }

    template<typename Coder>
    inline static void Transfer(value_type& data, Coder &coder) {
        coder.AddMetaFlag (kFlowMappingStyle);
        coder.transfer(data.x, "x");
        coder.transfer(data.y, "y");
        coder.transfer(data.z, "z");
        coder.transfer(data.w, "w");
    }
};

template<>
struct SerializeTraits<Vector3f> : public SerializeTraitsBase<Vector3f> {

    typedef Vector3f value_type;

    inline static const char* GetTypeString () { return "Vector3f"; }

    template<typename Coder>
    inline static void Transfer(value_type& data, Coder &coder) {
        coder.AddMetaFlag (kFlowMappingStyle);
        coder.transfer(data.x, "x");
        coder.transfer(data.y, "y");
        coder.transfer(data.z, "z");
    }
};

template<>
struct SerializeTraits<Quaternionf> : public SerializeTraitsBase<Quaternionf> {

    inline static const char* GetTypeString () { return "Quaternionf"; }

    template<typename Coder>
    inline static void Transfer(value_type& data, Coder &coder) {
        coder.AddMetaFlag (kFlowMappingStyle);
        coder.transfer(data.x, "x");
        coder.transfer(data.y, "y");
        coder.transfer(data.z, "z");
        coder.transfer(data.w, "w");
    }
};

}


#endif//OJOIE_MATH_H
