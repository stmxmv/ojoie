//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_SPHERE_HPP
#define OJOIE_SPHERE_HPP

#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {

struct Sphere {

    std::vector<Vector3f> vertices;
    std::vector<Vector3f> normals;
    std::vector<UInt16> indices;
    std::vector<Vector2f> uvs;

    Sphere(float radius, int latDiv, int longDiv) {
        Vector3f base{ 0.f, radius, 0.f };
        float latRadians = Math::pi<float>() / (float)latDiv;
        float longRadians = 2.f * Math::pi<float>() / (float)longDiv;

        size_t size = (latDiv - 1) * (longDiv + 1) + 2;;
        vertices.reserve(size);
        normals.reserve(size);
        indices.reserve(size * 6);

        for (int i = 1; i < latDiv; ++i) {
            Vector3f latBase = Math::rotateX(base, latRadians * (float)i);
            for (int j = 0; j <= longDiv; ++j) {
                Vector3f pos = Math::rotateY(latBase, longRadians * (float)j);
                vertices.push_back(pos);
                normals.push_back(Math::normalize(pos));

                // Generate UVs
                float u = (float)j / (float)(longDiv);
                float v = (float)i / (float)(latDiv - 1);
                uvs.emplace_back(u, v);
            }
        }

        size_t northIndex = vertices.size();
        vertices.push_back(base);
        normals.push_back(base);
        // UV for North pole
        uvs.emplace_back(0.5f, 1.0f);

        size_t southIndex = northIndex + 1;
        vertices.push_back(-base);
        normals.push_back(-base);
        // UV for South pole
        uvs.emplace_back(0.5f, 0.0f);


        auto index = [longDiv](int i, int j) {
            return i * (longDiv + 1) + j; // Adjusted for the additional vertex
        };

        // clang-format off
        for (int i = 0; i < latDiv - 2; ++i) {
            for (int j = 0; j < longDiv; ++j) {
                indices.push_back(index(    i,     j));
                indices.push_back(index(i + 1,     j));
                indices.push_back(index(    i, j + 1));
                indices.push_back(index(    i, j + 1));
                indices.push_back(index(i + 1,     j));
                indices.push_back(index(i + 1, j + 1));
            }
//            /// wrap band
//            indices.push_back(index(    i, longDiv - 1));
//            indices.push_back(index(i + 1, longDiv - 1));
//            indices.push_back(index(    i,           0));
//            indices.push_back(index(    i,           0));
//            indices.push_back(index(i + 1, longDiv - 1));
//            indices.push_back(index(i + 1,           0));
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

AN_API Mesh *GetSphereMesh();

}

#endif//OJOIE_SPHERE_HPP
