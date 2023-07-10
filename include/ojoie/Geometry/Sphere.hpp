//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_SPHERE_HPP
#define OJOIE_SPHERE_HPP

#include <ojoie/Math/Math.hpp>

namespace AN {

struct Sphere {

    std::vector<Vector3f> vertices;
    std::vector<Vector3f> normals;
    std::vector<UInt16> indices;

    Sphere(float radius, int latDiv, int longDiv) {
        Vector3f base{ 0.f, radius, 0.f };
        float latRadians = Math::pi<float>() / (float)latDiv;
        float longRadians = 2.f * Math::pi<float>() / (float)longDiv;

        size_t size = (latDiv - 1) * longDiv + 2;
        vertices.reserve(size);
        normals.reserve(size);
        indices.reserve(size * 6);

        for (int i = 1; i < latDiv; ++i) {
            Vector3f latBase = Math::rotateX(base, latRadians * (float)i);
            for (int j = 0; j < longDiv; ++j) {
                Vector3f pos = Math::rotateY(latBase, longRadians * (float)j);
                vertices.push_back(pos);
                normals.push_back(Math::normalize(pos));
            }
        }

        size_t northIndex = vertices.size();
        vertices.push_back(base);
        normals.push_back(base);

        size_t southIndex = northIndex + 1;
        vertices.push_back(-base);
        normals.push_back(-base);

        auto index = [longDiv](int i, int j) {
            return i * longDiv + j;
        };

        // clang-format off
        for (int i = 0; i < latDiv - 2; ++i) {
            for (int j = 0; j < longDiv - 1; ++j) {
                indices.push_back(index(    i,     j));
                indices.push_back(index(i + 1,     j));
                indices.push_back(index(    i, j + 1));
                indices.push_back(index(    i, j + 1));
                indices.push_back(index(i + 1,     j));
                indices.push_back(index(i + 1, j + 1));
            }
            /// wrap band
            indices.push_back(index(    i, longDiv - 1));
            indices.push_back(index(i + 1, longDiv - 1));
            indices.push_back(index(    i,           0));
            indices.push_back(index(    i,           0));
            indices.push_back(index(i + 1, longDiv - 1));
            indices.push_back(index(i + 1,           0));
        }

        /// cap fans
        for (int i = 0; i < longDiv; ++i) {
            /// north
            indices.push_back(northIndex);
            indices.push_back(index(0,     i));
            indices.push_back(index(0, i + 1));

            /// south
            indices.push_back(index(latDiv - 2, i + 1));
            indices.push_back(index(latDiv - 2,     i));
            indices.push_back(southIndex);
        }

        /// north
        indices.push_back(northIndex);
        indices.push_back(index(0, longDiv - 1));
        indices.push_back(index(0,           0));

        /// south
        indices.push_back(index(latDiv - 2,           0));
        indices.push_back(index(latDiv - 2, longDiv - 1));
        indices.push_back(southIndex);

        // clang-format on
    }



};


}

#endif//OJOIE_SPHERE_HPP
