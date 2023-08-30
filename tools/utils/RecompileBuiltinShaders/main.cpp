//
// Created by aojoie on 6/18/2023.
//

#include <iostream>

#include <ojoie/Render/Shader/Shader.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Core/Configuration.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/IO/FileOutputStream.hpp>

#include <format>
#include <filesystem>

#include <ojoie/Input/InputManager.hpp>

using namespace AN;
using std::cout, std::endl;

int main() {

    InitializeRenderContext(AN::kGraphicsAPID3D11);


    const char *rootPath = AN_SHADER_ROOT;

    for (auto const& entry : std::filesystem::recursive_directory_iterator{ rootPath }) {
        if (entry.is_directory() || entry.path().extension() != ".shader") continue;

        Shader *shader = NewObject<Shader>();

        ANAssert(shader->initWithScript(entry.path().string()));

        YamlEncoder yamlEncoder;
        File file;
        file.open(std::format("Data/Assets/Shaders/{}.asset", entry.path().stem().string()).c_str(), AN::kFilePermissionWrite);
        FileOutputStream fileOutputStream(file);

        shader->redirectTransferVirtual(yamlEncoder);

        yamlEncoder.outputToStream(fileOutputStream);

        DestroyObject(shader);

    }

    DeallocRenderContext();


    return 0;
}