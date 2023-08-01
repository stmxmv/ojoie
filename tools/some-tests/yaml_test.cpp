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


class AnyObject {
    int m_Data;
public:
    AnyObject() : m_Data(42) {}

    void assertValue(int v) {
        assert(v == m_Data);
    }
};


namespace AN {

struct AnyObject_m_Data_Tag : Access::TagBase<AnyObject_m_Data_Tag> {};

template struct Access::Accessor<AnyObject_m_Data_Tag, &AnyObject::m_Data>;

}

struct TestStruct {

    char binaryData[2] = { 0x1a, 0x4f };
    int value = 13;
    float fvalue = 1.3;
    std::string stringValue = "ANIsGood";

    std::vector<int> vec{ 13, 13, 24, 24 };

    std::unordered_map<int, std::string> map{ { 2, "123"}, { 13, "I Am AN"} };

    std::vector<int> empty_vec{};

    DECLARE_SERIALIZE(TestStruct)

    void clear() {
        binaryData[0] = 0;
        binaryData[1] = 0;
        value = 0;
        fvalue = 0;
        stringValue.clear();
        vec.clear();
        map.clear();
    }
};

template<typename Coder>
void TestStruct::transfer(Coder &coder) {
    coder.transfer(value, "value");

    coder.transfer(stringValue, "ANString");
    size_t size = sizeof(binaryData);
    coder.transferTypeless(size, "binaryDataSize");
    coder.transferTypelessData(binaryData, sizeof(binaryData));
    coder.transfer(vec, "array");
    coder.transfer(fvalue, "fvalue");
    coder.transfer(map, "map");
    coder.transfer(empty_vec, "emptyVec");
}

INSTANTIATE_TEMPLATE_TRANSFER(TestStruct)


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
    file2.open(std::format("Data/Assets/{}.asset", outputName).c_str(), AN::kFilePermissionWrite);
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
    file2.open(std::format("Data/Assets/{}.asset", outputName).c_str(), AN::kFilePermissionWrite);
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
    file.open("Data/Assets/BusterDrone.asset", AN::kFilePermissionWrite);
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


//    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\ojoie\\tools\\ojoieEditor\\Assets\\Textures\\file.png", "FileIconTex");
//
//    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\ojoie\\tools\\ojoieEditor\\Assets\\Textures\\folder.png", "FolderIconTex");
//
//    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\ojoie\\tools\\ojoieEditor\\Assets\\Textures\\folder_empty.png", "FolderEmptyIconTex");

    encodeTexture("C:\\Users\\Aleudillonam\\CLionProjects\\Assets\\skybox\\back.jpg", "SkyboxBack");

    return 0;
//
//    const char *cubeTextures[6] = {
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\right.jpg",
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\left.jpg",
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\top.jpg",
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\bottom.jpg",
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\front.jpg",
//        "C:\\Users\\aojoie\\CLionProjects\\Assets\\skybox\\back.jpg"
//    };

//    encodeTextureCube(cubeTextures, "SkyBox");
    encoderObject();
    return 0;

    File file;
    file.open("中文路径/temp.txt", AN::kFilePermissionWrite);

    YamlEncoder yamlEncoder;
    TestStruct testStruct;
    yamlEncoder.transfer(testStruct, "testStruct");
    yamlEncoder.transfer(testStruct, "testStruct2");
    std::string result;
//    yamlEncoder.outputToString(result);

//    cout << result << endl;

    FileOutputStream fileOutputStream(file);
    yamlEncoder.outputToStream(fileOutputStream);

    file.open("temp.txt", AN::kFilePermissionRead);
    FileInputStream fileInputStream(file);
    YamlDecoder yamlDecoder(fileInputStream);

    testStruct.clear();
    yamlDecoder.transfer(testStruct, "testStruct");
    testStruct.clear();
    yamlDecoder.transfer(testStruct, "testStruct2");

    return 0;
}