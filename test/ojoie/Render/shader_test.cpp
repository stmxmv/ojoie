//
// Created by aojoie on 4/14/2023.
//

#include <gtest/gtest.h>
#include <iostream>

#include <ojoie/Render/Shader/Shader.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Core/Configuration.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/IO/FileOutputStream.hpp>

using namespace AN;
using std::cout, std::endl;


TEST(Shader, init) {

    InitializeRenderContext(AN::kGraphicsAPID3D11);

    Shader *shader = NewObject<Shader>();


    const char *filePath = "C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\Skybox-Procedural.shader";

    ASSERT_TRUE(shader->initWithScript(filePath));

    YamlEncoder yamlEncoder;
    File file;
    file.open("Data/Assets/Skybox-Procedural.asset", AN::kFilePermissionWrite);
    FileOutputStream fileOutputStream(file);

    shader->redirectTransferVirtual(yamlEncoder);

    yamlEncoder.outputToStream(fileOutputStream);

    DestroyObject(shader);

    DeallocRenderContext();
}
