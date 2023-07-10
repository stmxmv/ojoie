//
// Created by aojoie on 4/24/2023.
//

#include "ojoie/Asset/ImportMesh.hpp"
#include "ojoie/BaseClasses/ObjectPtr.hpp"
#include "ojoie/Render/RenderTypes.hpp"
#include <ojoie/Asset/FBXImporter.hpp>

#include <ojoie/Render/Mesh/Mesh.hpp>

#include <iostream>

using namespace AN;

using std::cout, std::endl;
int main(int argc, char** argv) {



    // Change the following filename to a suitable filename value.
    const char* lFilename = "./Resources/Models/sphere.fbx";

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

    ObjectPtr<Mesh> mesh = MakeObjectPtr<Mesh>();
    mesh->init();
    mesh->resizeVertices(importMesh.positions.size(), kShaderChannelVertex | kShaderChannelTexCoord0 | kShaderChannelNormal);

    mesh->setVertices(importMesh.positions.data(), importMesh.positions.size());
    mesh->setUV(0, importMesh.texcoords[0].data(), importMesh.texcoords[0].size());
    mesh->setNormals(importMesh.normals.data(), importMesh.normals.size());
    mesh->setTangents(importMesh.tangents.data(), importMesh.tangents.size());

    mesh->setSubMeshCount(importMesh.subMeshes.size());
    for (int i = 0; i < importMesh.subMeshes.size(); ++i) {
        mesh->setIndices(importMesh.subMeshes[i].indices.data(), importMesh.subMeshes[i].indices.size(), i);
    }


    return 0;
}
