//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_CUBE_HPP
#define OJOIE_CUBE_HPP


#include <ojoie/Render/Mesh/Mesh.hpp>
#include <array>


namespace AN {

struct Cube {
    // Vertices of a unit cube centered at origin, sides aligned with axes
    std::array<Vector3f , 24> vertices = {
        Vector3f{ -0.5, -0.5, -0.5 },
        { 0.5, 0.5, -0.5 },
        { 0.5, -0.5, -0.5 },
        { -0.5, 0.5, -0.5 },
        { -0.5, -0.5, 0.5 },
        { 0.5, -0.5, 0.5 },
        { 0.5, 0.5, 0.5 },
        { -0.5, 0.5, 0.5 },
        { -0.5, 0.5, 0.5 },
        { -0.5, 0.5, -0.5 },
        { -0.5, -0.5, -0.5 },
        { -0.5, -0.5, 0.5 },
        { 0.5, 0.5, 0.5 },
        { 0.5, -0.5, -0.5 },
        { 0.5, 0.5, -0.5 },
        { 0.5, -0.5, 0.5 },
        { -0.5, -0.5, -0.5 },
        { 0.5, -0.5, -0.5 },
        { 0.5, -0.5, 0.5 },
        { -0.5, -0.5, 0.5 },
        { -0.5, 0.5, -0.5 },
        { 0.5, 0.5, 0.5 },
        { 0.5, 0.5, -0.5 },
        { -0.5, 0.5, 0.5 },
    };

    // Indices for the triangles that make up the cube
    std::array<UInt16, 36> indices = {
        0, 1, 2, 1, 0, 3,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 13, 12, 15,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 21, 20, 23
    };

    // Normals for each face of the cube
    std::array<Vector3f, 24> normals = {
        Vector3f{ 0, 0, -1 },
        { 0, 0, -1 },
        { 0, 0, -1 },
        { 0, 0, -1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { -1, 0, 0 },
        { -1, 0, 0 },
        { -1, 0, 0 },
        { -1, 0, 0 },
        { 1, 0, 0 },
        { 1, 0, 0 },
        { 1, 0, 0 },
        { 1, 0, 0 },
        { 0, -1, 0 },
        { 0, -1, 0 },
        { 0, -1, 0 },
        { 0, -1, 0 },
        { 0, 1, 0 },
        { 0, 1, 0 },
        { 0, 1, 0 },
        { 0, 1, 0 },
    };

    // UVs for each vertex
    std::array<Vector2f, 24> texcoord0s = {
        Vector2f{ 0, 0 },
        { 1, 1 },
        { 1, 0 },
        { 0, 1 },
        { 0, 0 },
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
        { 1, 0 },
        { 0, 1 },
        { 1, 1 },
        { 0, 0 },
        { 0, 1 },
        { 1, 1 },
        { 1, 0 },
        { 0, 0 },
        { 0, 1 },
        { 1, 0 },
        { 1, 1 },
        { 0, 0 },
    };
};


AN_API Mesh *GetCubeMesh();


} // namespace AN
#endif//OJOIE_CUBE_HPP
