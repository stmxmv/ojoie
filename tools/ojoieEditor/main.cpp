//
// Created by aojoie on 4/22/2023.
//


#define IMGUI_DEFINE_MATH_OPERATORS

#include <iostream>
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Screen.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Render/Mesh/MeshRenderer.hpp>
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/IO/FileInputStream.hpp>
#include <ojoie/Render/RenderManager.hpp>
#include <ojoie/Misc/ResourceManager.hpp>

#include "AppDelegate.hpp"

using std::cout, std::endl;
using namespace AN;
using namespace AN::Editor;

namespace unused {
class AppDelegate : public AN::ApplicationDelegate {

public:
    Actor *createCubeActor() {
        Actor *cubeActor = NewObject<Actor>();
        cubeActor->init("Cube");

        File file;
        FileInputStream fileInputStream(file);
        file.Open("Data/Assets/BusterDrone.asset", kFilePermissionRead);
        YamlDecoder meshDecoder(fileInputStream);
        Mesh *mesh = NewObject<Mesh>();
        mesh->redirectTransferVirtual(meshDecoder);
        ANAssert(mesh->initAfterDecode());

        mesh->createVertexBuffer();

        MeshRenderer *meshRenderer = cubeActor->addComponent<MeshRenderer>();
        meshRenderer->setMesh(mesh);

        Shader *shader = NewObject<Shader>();
        file.Open("Data/Assets/DefaultShader.asset", AN::kFilePermissionRead);
        YamlDecoder decoder(fileInputStream);
        shader->redirectTransferVirtual(decoder);

        ANAssert(shader->initAfterDecode());
        ANAssert(shader->createGPUObject());

        Material *material = NewObject<Material>();
        material->init(shader, "DefaultMaterial");

        file.Open("Data/Assets/BusterDroneTex.asset", kFilePermissionRead);
        YamlDecoder texDecoder(fileInputStream);
        Texture2D *texture = NewObject<Texture2D>();
        texture->redirectTransferVirtual(texDecoder);
        ANAssert(texture->initAfterDecode());
        texture->uploadToGPU(true);

        material->setTexture("_DiffuseTex", texture);

        {
            file.Open("Data/Assets/BusterDroneNorm.asset", kFilePermissionRead);
            YamlDecoder normalMapDecoder(fileInputStream);
            Texture2D *NormalMap = NewObject<Texture2D>();
            NormalMap->redirectTransferVirtual(normalMapDecoder);
            ANAssert(NormalMap->initAfterDecode());
            NormalMap->uploadToGPU(false);

            material->setTexture("_NormalMap", NormalMap);
        }

        meshRenderer->setMaterial(0, material);

        return cubeActor;
    }
};
}


int main(int argc, const char *argv[]) {

    Application &app = Application::GetSharedApplication();
    AppDelegate *appDelegate = new AppDelegate();
    app.setDelegate(appDelegate);
    appDelegate->release();

    app.setName("ojoie editor");
    app.run(argc, argv);
    return 0;
}