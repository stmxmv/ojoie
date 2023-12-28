//
// Created by aojoie on 5/6/2023.
//


#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/Serialize/SerializeDefines.h>
#include <ojoie/Asset/FBXImporter.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>

#include <ojoie/Render/TextureCube.hpp>
#include <ojoie/Render/TextureLoader.hpp>

#include "ojoie/Object/ObjectPtr.hpp"
#include <iostream>
#include <ojoie/HAL/File.hpp>
#include <ojoie/IO/FileInputStream.hpp>
#include <ojoie/IO/FileOutputStream.hpp>
#include <ojoie/Utility/String.hpp>

#include <ojoie/Template/Access.hpp>

using std::cout, std::endl;

using namespace AN;


void encodeTexture(const char *path, const char *outputName, bool sRGB = true) {
    auto result = TextureLoader::LoadTexture(path, sRGB);
    Texture2D *texture = NewObject<Texture2D>();
    TextureDescriptor textureDescriptor;
    textureDescriptor.width = result.getWidth();
    textureDescriptor.height = result.getHeight();
    textureDescriptor.pixelFormat = result.getPixelFormat();
    textureDescriptor.mipmapLevel = result.getMipmapLevel();

    ANAssert(texture->init(textureDescriptor));
    texture->setPixelData(result.getData());
    texture->setName(outputName);

    YamlEncoder yamlEncoder2;
    File file2;
    file2.Open(std::format("Data/Assets/{}.asset", outputName).c_str(), AN::kFilePermissionWrite);
    FileOutputStream fileOutputStream2(file2);
    texture->redirectTransferVirtual(yamlEncoder2);
    yamlEncoder2.outputToStream(fileOutputStream2);

    DestroyObject(texture);
}

void encodeTextureCube(const char *path[6], const char *outputName, bool sRGB = true) {
    std::vector<Texture2D *> sourcesTextures;

    TextureDescriptor textureDescriptor;
    for (int i = 0; i < 6; ++i) {
        auto result = TextureLoader::LoadTexture(path[i], sRGB);
        Texture2D *texture = NewObject<Texture2D>();
        textureDescriptor.width = result.getWidth();
        textureDescriptor.height = result.getHeight();
        textureDescriptor.pixelFormat = result.getPixelFormat();
        textureDescriptor.mipmapLevel = result.getMipmapLevel();

        ANAssert(texture->init(textureDescriptor));
        texture->setPixelData(result.getData());
        texture->setName(outputName);

        sourcesTextures.push_back(texture);
    }

    TextureCube *textureCube = NewObject<TextureCube>();
    ANAssert(textureCube->init(textureDescriptor));

    for (int i = 0; i < 6; ++i) {
        textureCube->setSourceTexture(i, sourcesTextures[i]);
    }

    textureCube->buildFromSources();

    YamlEncoder yamlEncoder2;
    File file2;
    file2.Open(std::format("Data/Assets/{}.asset", outputName).c_str(), AN::kFilePermissionWrite);
    FileOutputStream fileOutputStream2(file2);
    textureCube->redirectTransferVirtual(yamlEncoder2);
    yamlEncoder2.outputToStream(fileOutputStream2);

    for (auto &item : sourcesTextures) {
        DestroyObject(item);
    }

    DestroyObject(textureCube);
}

void encoderObject() {
    const char* lFilename = "C:\\Users\\Aleudillonam\\CLionProjects\\3d-models\\BusterDrone\\BusterDrone.fbx";

    FBXImporter importer;

    if (!importer.isValid()) {
        std::cout << "FBXPlugin not support" << std::endl;
        exit(1);
    }

    ANAssert(importer.loadScene(lFilename, nullptr));
    importer.importMesh(nullptr);
    auto meshes = importer.getImportMeshes();

    ANAssert(meshes.size() > 0);
    const ImportMesh &importMesh = meshes[0];

    ObjectPtr<Mesh> mesh = MakeObjectPtr<Mesh>();
    mesh->init();
    mesh->resizeVertices(importMesh.positions.size(), kShaderChannelVertex | kShaderChannelTexCoord0 |
                                                              kShaderChannelNormal | kShaderChannelTangent);

    mesh->setVertices(importMesh.positions.data(), importMesh.positions.size());
    mesh->setUV(0, importMesh.texcoords[0].data(), importMesh.texcoords[0].size());
    mesh->setNormals(importMesh.normals.data(), importMesh.normals.size());
    mesh->setTangents(importMesh.tangents.data(), importMesh.tangents.size());

    mesh->setSubMeshCount(importMesh.subMeshes.size());
    for (int i = 0; i < importMesh.subMeshes.size(); ++i) {
        mesh->setIndices(importMesh.subMeshes[i].indices.data(), importMesh.subMeshes[i].indices.size(), i);
    }

    File file;
    file.Open("Data/Assets/BusterDrone.asset", AN::kFilePermissionWrite);
    FileOutputStream fileOutputStream(file);

    YamlEncoder yamlEncoder;
    mesh->redirectTransferVirtual(yamlEncoder);
    yamlEncoder.outputToStream(fileOutputStream);

    size_t textureSize;
    UInt8 *textureData = importer.getTextureData(0, textureSize);

    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\3d-models\\BusterDrone\\body_albedo.png", "BusterDroneTex");
    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\3d-models\\BusterDrone\\body_normal.png", "BusterDroneNorm", false);
}

int main(int argc, const char * argv[]) {

    encodeTexture(std::format("{}/Textures/file.png", EDITOR_RESOURCE_ROOT).c_str(), "FileIconTex");
    encodeTexture(std::format("{}/Textures/folder.png", EDITOR_RESOURCE_ROOT).c_str(), "FolderIconTex");
    encodeTexture(std::format("{}/Textures/folder_empty.png", EDITOR_RESOURCE_ROOT).c_str(), "FolderEmptyIconTex");

//    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\Assets\\skybox\\back.jpg", "SkyboxBack");
    return 0;
}