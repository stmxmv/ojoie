//
// Created by aojoie on 4/24/2023.
//

#include "ojoie/Asset/ImportMesh.hpp"
#include "ojoie/Object/ObjectPtr.hpp"
#include "ojoie/Render/RenderTypes.hpp"
#include <ojoie/Asset/FBXImporter.hpp>

#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Math/Math.hpp>

#include <iostream>

using namespace AN;

using std::cout, std::endl;
int main(int argc, char** argv) {

    Vector3f angles(0.F, 30.F, 60.F);

    Quaternionf q = Math::toQuat(Math::eulerAngleYXZ(Math::radians(angles.y), Math::radians(angles.x), Math::radians(angles.z)));

    cout << q.x << ' ' << q.y << ' ' << q.z << ' ' << q.w << endl;

    float yaw, pitch, roll;
    Math::extractEulerAngleYXZ(Math::toMat4(q), yaw, pitch, roll);

    Vector3f angles1 = { Math::degrees(pitch), Math::degrees(yaw), Math::degrees(roll) };

    cout << angles1.x << ' ' << angles1.y << ' ' << angles1.z << endl;

    return 0;

    // Change the following filename to a suitable filename value.
    const char* lFilename = "./Resources/Models/TestCharacter.fbx";

    FBXImporter importer;

    if (!importer.isValid()) {
        std::cout << "FBXPlugin not support" << std::endl;
        return 1;
    }

    ANAssert(importer.loadScene(lFilename, nullptr));
    importer.importMesh(nullptr);
    auto meshes = importer.getImportMeshes();

    ANAssert(meshes.size() > 0);
    const ImportMesh &importMesh = meshes[0];

//    ObjectPtr<Mesh> mesh = MakeObjectPtr<Mesh>();
//    mesh->init();
//    mesh->resizeVertices(importMesh.positions.size(), kShaderChannelVertex | kShaderChannelTexCoord0 | kShaderChannelNormal);
//
//    mesh->setVertices(importMesh.positions.data(), importMesh.positions.size());
//    mesh->setUV(0, importMesh.texcoords[0].data(), importMesh.texcoords[0].size());
//    mesh->setNormals(importMesh.normals.data(), importMesh.normals.size());
//    mesh->setTangents(importMesh.tangents.data(), importMesh.tangents.size());
//
//    mesh->setSubMeshCount(importMesh.subMeshes.size());
//    for (int i = 0; i < importMesh.subMeshes.size(); ++i) {
//        mesh->setIndices(importMesh.subMeshes[i].indices.data(), importMesh.subMeshes[i].indices.size(), i);
//    }


    return 0;
}
