//
// Created by Aleudillonam on 7/27/2023.
//
#include "Geometry/Sphere.hpp"

namespace AN {

Mesh *GetSphereMesh() {
    static Mesh *mesh;
    static std::once_flag once;
    
    std::call_once(once, [] {

        mesh = NewObject<Mesh>();
        mesh->init();
        mesh->setName("Sphere");

        Sphere sphere(0.5f, 30, 30);
        mesh->resizeVertices(sphere.vertices.size(), VERTEX_FORMAT3(Vertex, TexCoord0, Normal));
        mesh->setSubMeshCount(1);

        mesh->setVertices(sphere.vertices.data(), sphere.vertices.size());
        mesh->setUV(0, sphere.uvs.data(), sphere.uvs.size());
        mesh->setNormals(sphere.normals.data(), sphere.normals.size());
        mesh->setIndices(sphere.indices.data(), sphere.indices.size(), 0);

        mesh->createVertexBuffer();
    });

    return mesh;
}

}