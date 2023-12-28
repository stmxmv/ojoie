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

struct ImportBone
{
    std::string name;
    int parent; // root is -1

    Vector3f    localPosition;
    Quaternionf localRotation;
    Vector3f    localScale;
};

struct ImportMesh {

    std::vector<Vector3f> positions;
    std::vector<Vector2f> texcoords[2];
    std::vector<Vector3f> normals;
    std::vector<Vector4f> tangents;

    std::vector<std::vector<Vector2f>> boneWeights;

    ImportBone rootBone;
    std::vector<ImportBone> bones;

    std::vector<ImportSubMesh> subMeshes;

    Vector3f    localPosition;
    Quaternionf localRotation;
    Vector3f    localScale;
};

}

#endif//OJOIE_IMPORTMESH_HPP
