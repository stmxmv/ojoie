//
// Created by Aleudillonam on 5/1/2023.
//

#ifndef OJOIE_IMPORTMESH_HPP
#define OJOIE_IMPORTMESH_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Math/Math.hpp>

#include <vector>

namespace AN {



struct ImportMaterial {

    UInt32 importTextureID;

};

struct ImportSubMesh {
    std::vector<UInt16> indices;
    ImportMaterial material;
};

struct ImportMesh {

    std::vector<Vector3f> positions;
    std::vector<Vector2f> texcoords[2];
    std::vector<Vector3f> normals;
    std::vector<Vector4f> tangents;

    std::vector<ImportSubMesh> subMeshes;
};

}

#endif//OJOIE_IMPORTMESH_HPP
