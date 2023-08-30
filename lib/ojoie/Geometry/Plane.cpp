//
// Created by Aleudillonam on 7/27/2023.
//
#include "Geometry/Plane.hpp"

namespace AN {

Mesh *GetPlaneMesh() {
    static Mesh *mesh;
    static std::once_flag once;
    
    std::call_once(once, [] {

        mesh = NewObject<Mesh>();
        mesh->init();
        mesh->setName("Plane");

        Plane plane;
        mesh->resizeVertices(plane.vertices.size(), VERTEX_FORMAT3(Vertex, TexCoord0, Normal));
        mesh->setSubMeshCount(1);

        mesh->setVertices(plane.vertices.data(), plane.vertices.size());
        mesh->setUV(0, plane.texcoord0s.data(), plane.texcoord0s.size());
        mesh->setNormals(plane.normals.data(), plane.normals.size());
        mesh->setIndices(plane.indices.data(), plane.indices.size(), 0);

        mesh->createVertexBuffer();
    });

    return mesh;
}

}