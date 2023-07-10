//
// Created by aojoie on 4/13/2023.
//

#include <format>
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <ojoie/Render/Shader/ShaderCompiler.hpp>
#include <ojoie/Render/PipelineReflection.hpp>
#include <ojoie/Utility/SourceFile.hpp>

using namespace AN;
using namespace AN::RC;
using std::cin, std::cout, std::endl;

TEST(Render, Shader) {

    SourceFile sourceFile;

    const char *filePath = "C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\vulkan\\commonTest.hlsl";

    std::filesystem::path path(filePath);
    auto fileName = path.filename().string();

    EXPECT_TRUE(sourceFile.init(filePath));


    ShaderCompiler     shaderCompiler;
    PipelineReflection pipelineReflection;

    const char *includes[] = {
            "C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\vulkan",
            "C:\\Users\\aojoie\\CLionProjects\\ojoie\\include\\ojoie\\ShaderLab\\stdlib"
    };

    std::vector<UInt8> spv = shaderCompiler.compileHLSLToCSO(kShaderStageVertex,
                                                               sourceFile.getBuffer(),
                                                               "vertex_main",
                                                               fileName.c_str(),
                                                               includes);

    EXPECT_TRUE(pipelineReflection.reflectCSO(spv.data(), spv.size(), kShaderStageVertex));

    spv = shaderCompiler.compileHLSLToCSO(kShaderStageFragment,
                                            sourceFile.getBuffer(),
                                            "fragment_main",
                                            fileName.c_str(),
                                            includes);

    sourceFile.close();

    EXPECT_TRUE(pipelineReflection.reflectCSO(spv.data(), spv.size(), kShaderStageFragment));

    cout << "vertex input" << endl;
    for (const ShaderVertexInput &input : pipelineReflection.getVertexInputs()) {
        cout << std::format("loc:{} type:{} dimension: {} semantic:{}",
                            input.location, (int) input.format, input.dimension, input.semantic)
             << endl;

        EXPECT_EQ(&input, pipelineReflection.getVertexInput(input.semantic.c_str()));
    }
    cout << "\n\n";
    cout << "resources" << endl;
    for (const AN::ShaderResource &resource : pipelineReflection.getResources()) {
        cout << std::format("set {} binding {} resource type {} {}",
                            resource.set,
                            resource.binding,
                            (int) resource.resourceType,
                            resource.name);
        if (resource.resourceType == kShaderResourceBufferUniform) {
            cout << " size " << resource.block.size;
        }
        cout << endl;
        EXPECT_EQ(&resource, pipelineReflection.getResource(resource.name.c_str(), resource.stage));
    }

    cout << '\n';

    ShaderPropertyList propertyMap = pipelineReflection.buildPropertyList();
    cout << "Shader properties" << endl;
    for (auto & prop : propertyMap) {
        if (prop.resourceType == AN::kShaderResourcePushConstant) {
            cout << std::format("pushConstant name: {} type: {} offset: {}, size: {}",
                                prop.name.string_view(), (int)prop.propertyType, prop.offset, prop.size);
        } else if (prop.resourceType == AN::kShaderResourceSpecializationConstant) {
            cout << std::format("name: {} constant_id: {} type: {}",
                                prop.name.string_view(), prop.constant_id, (int)prop.propertyType);
        } else {
            cout << std::format("name: {} set: {} binding: {} offset: {} size: {}",
                                prop.name.string_view(), prop.set, prop.binding, prop.offset, prop.size);
        }

        cout << " stage: ";
        if (prop.stage & kShaderStageVertex) {
            cout << "vertex ";
        }
        if (prop.stage & kShaderStageFragment) {
            cout << "fragment ";
        }
        cout << '\n';
    }
    cout.flush();
    EXPECT_FALSE(propertyMap.empty());

}