//
// Created by Aleudillonam on 8/7/2022.
//

#include "Render/Model.hpp"
#include "Node/StaticModelNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"

namespace AN {

static RenderPipeline pipeline;

static const char *vertexShaderSource = "#version 430 core\n"
                                   "\n"
                                   "layout (location = 0) in vec3 aPos;\n"
                                   "layout (location = 1) in vec2 aTexCoord;\n"
                                   "\n"
                                   "out vec2 TexCoord;\n"
                                   "\n"
                                   "uniform mat4 model;\n"
                                   "uniform mat4 view;\n"
                                   "uniform mat4 projection;\n"
                                   "\n"
                                   "void main() {\n"
                                   "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                                   "    TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
                                   "}";

static const char *fragmentShaderSource = "#version 430 core\n"
                                          "\n"
                                          "out vec4 FragColor;\n"
                                          "\n"
                                          "in vec2 TexCoord;\n"
                                          "\n"
                                          "struct Material {\n"
                                          "    sampler2D texture_diffuse1;\n"
                                          "    sampler2D texture_diffuse2;\n"
                                          "    sampler2D texture_diffuse3;\n"
                                          "    sampler2D texture_specular1;\n"
                                          "    sampler2D texture_specular2;\n"
                                          "};\n"
                                          "\n"
                                          "uniform Material material;\n"
                                          "\n"
                                          "\n"
                                          "void main() {\n"
                                          "    vec4 sampleColor = texture(material.texture_diffuse1, TexCoord);\n"
                                          "    if (sampleColor.a < 0.1f) {\n"
                                          "        discard;\n"
                                          "    }\n"
                                          "    FragColor = sampleColor;\n"
                                          "}";



struct StaticModelNode::Impl {
    std::atomic_int ins_cnt{};
    Model model;
};

StaticModelNode::StaticModelNode() : impl(new Impl{}) {
    _needsRender = true;
}

StaticModelNode::~StaticModelNode() {
    if (--impl->ins_cnt == 0) {
        Dispatch::async(Dispatch::Render, [impl = impl] {
            delete impl;
        });
    }
}

bool StaticModelNode::init(const char *modelPath) {
    if (Super::init()) {
        Dispatch::async(Dispatch::Render, [modelPath = std::string(modelPath), _self = shared_from_this()] {

            Self *self = (Self *)_self.get();

            if (!self->impl->model.init(modelPath.c_str())) {
                ANLog("model init fail with path %s", modelPath.c_str());
            }

            ++self->impl->ins_cnt;
        });

        return true;
    }

    return false;
}
void StaticModelNode::render(const RenderContext &context) {
    Super::render(context);

    static bool isShaderInited = false;
    if (!isShaderInited) {
        isShaderInited = true;

        ANAssert(pipeline.initWithSource(vertexShaderSource, fragmentShaderSource));


        GetGame().registerCleanupTask([] {
            Dispatch::async(Dispatch::Render, [] {
                pipeline.deinit();
            });
        });
    }


    pipeline.activate();

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    pipeline.setMat4("projection", cameraNode->getProjectionMatrix());
    pipeline.setMat4("view", cameraNode->getViewMatrix());
    pipeline.setMat4("model", getModelViewMatrix());

    impl->model.render(pipeline);
}


}