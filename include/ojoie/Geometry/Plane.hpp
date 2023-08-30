//
// Created by Aleudillonam on 7/30/2023.
//

#pragma once

#include <ojoie/Render/Mesh/Mesh.hpp>
#include <array>


namespace AN {

struct Plane {
    std::array<Vector3f, 4> vertices = {
        Vector3f { 5.f, 0.f, 5.f },
        { 5.f, 0.f, -5.f },
        { -5.f, 0.f, -5.f },
        { -5.f, 0.f, 5.f }
    };

    std::array<UInt16, 6> indices = {
        0, 1, 2, 0, 2, 3
    };

    std::array<Vector3f, 4> normals = {
        Vector3f { 0.f, 1.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 1.f, 0.f }
    };

    std::array<Vector2f, 4> texcoord0s = {
        Vector2f{ 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 }
    };

};

AN_API Mesh *GetPlaneMesh();

}