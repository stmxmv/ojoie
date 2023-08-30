//
// Created by Aleudillonam on 7/27/2023.
//

#include "Geometry/Cube.hpp"

namespace AN {


Mesh *GetCubeMesh() {
    static Mesh *mesh;
    static std::once_flag once;

    std::call_once(once, [] {

        mesh = NewObject<Mesh>();
        mesh->init();
        mesh->setName("Cube");

        Cube cube;
        mesh->resizeVertices(cube.vertices.size(), VERTEX_FORMAT3(Vertex, TexCoord0, Normal));
        mesh->setSubMeshCount(1);

        mesh->setVertices(cube.vertices.data(), cube.vertices.size());
        mesh->setUV(0, cube.texcoord0s.data(), cube.texcoord0s.size());
        mesh->setNormals(cube.normals.data(), cube.normals.size());
        mesh->setIndices(cube.indices.data(), cube.indices.size(), 0);

        mesh->createVertexBuffer();
    });

    return mesh;
}

}