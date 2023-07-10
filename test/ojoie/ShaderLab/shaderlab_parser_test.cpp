//
// Created by aojoie on 4/14/2023.
//

#include <gtest/gtest.h>
#include <iostream>
#include <ojoie/ShaderLab/Lexer.hpp>
#include <ojoie/ShaderLab/Parser.hpp>
#include <ojoie/Utility/SourceFile.hpp>

#include <ojoie/Render/Shader/ShaderCompiler.hpp>
#include <ojoie/Render/private/vulkan/PipelineReflection.hpp>

using namespace AN::ShaderLab;
using namespace AN;
using namespace AN::RC;
using namespace AN::VK;

using std::cout, std::endl;

class ParserTest : public testing::Test {

public:
    inline static SourceFile sourceFile;

    Lexer  *lexer;
    Parser *parser;

protected:
    static void SetUpTestSuite() {
        ASSERT_TRUE(sourceFile.init("C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\test.shader"));
    }

    static void TearDownTestSuite() {
        sourceFile.close();
    }

    virtual void SetUp() override {
        lexer = new Lexer(sourceFile.getBuffer());
        parser = new Parser(*lexer);
    }

    virtual void TearDown() override {
        delete parser;
        delete lexer;
    }
};

TEST_F(ParserTest, parse) {

    ShaderInfo shaderInfo = parser->parse();
    if (parser->hasError()) {
        cout << parser->getError().getDescription() << endl;
    }

    ASSERT_FALSE(parser->hasError());

    /// compile the first subshader first pass

    ShaderCompiler     shaderCompiler;
    PipelineReflection pipelineReflection;

    std::string realSource;
    realSource.reserve(shaderInfo.subShaderHLSLIncludes[0].size() + shaderInfo.passHLSLSources[0][0].size());
    realSource.append(shaderInfo.subShaderHLSLIncludes[0]);
    realSource.append("\r\n");
    realSource.append(shaderInfo.passHLSLSources[0][0]);

    const char *includes[] = {
            "C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\vulkan",
            "C:\\Users\\aojoie\\CLionProjects\\ojoie\\include\\ojoie\\ShaderLab\\stdlib"
    };

    std::vector<UInt8> spv = shaderCompiler.compileHLSLToSPIRV(kShaderStageVertex,
                                                               realSource.c_str(),
                                                               "vertex_main",
                                                               shaderInfo.name.c_str(),
                                                               includes);
    ASSERT_FALSE(spv.empty());
    EXPECT_TRUE(pipelineReflection.reflectSPIRV(spv.data(), spv.size()));

    spv = shaderCompiler.compileHLSLToSPIRV(kShaderStageFragment,
                                            realSource.c_str(),
                                            "fragment_main",
                                            shaderInfo.name.c_str(),
                                            includes);

    ASSERT_FALSE(spv.empty());
    EXPECT_TRUE(pipelineReflection.reflectSPIRV(spv.data(), spv.size()));

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
        EXPECT_EQ(&resource, pipelineReflection.getResource(resource.name.c_str()));
    }

    cout << '\n';

    PropertyMap propertyMap = pipelineReflection.buildPropertyMap();
    cout << "Shader properties" << endl;
    for (auto &[_, prop] : propertyMap) {
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
