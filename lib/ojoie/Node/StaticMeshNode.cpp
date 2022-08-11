//
// Created by Aleudillonam on 8/3/2022.
//
#include "Node/StaticMeshNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"
#include "Render/RenderPipeline.hpp"
#include <glad/glad.h>

namespace AN {

struct StaticMeshNode::Impl {

    std::atomic_int ins_cnt{};

    Math::vec3 _color;

    /// \RenderActor
    Mesh mesh;

};

StaticMeshNode::StaticMeshNode() : impl(new Impl{}) {
    _needsRender = true;
}

StaticMeshNode::~StaticMeshNode() {
    if (--impl->ins_cnt == 0) {
        Dispatch::async(Dispatch::Render, [impl = impl]() mutable {
            delete impl;
        });
    }
}

static ShaderLibrary vertexLibrary;
static ShaderLibrary fragmentLibrary;
static ShaderLibrary texturedFragmentLibrary;
static RenderPipeline pipeline;
static RenderPipeline texturedPipeline;

static const char *vertexShaderSource = "#version 430 core\n"
                                          "\n"
                                          "layout (location = 0) in vec3 aPos;\n"
                                          "layout (location = 1) in vec3 aNormal;\n"
                                          "layout (location = 2) in vec2 aTexCoord;\n"
                                          "\n"
                                          "out vec3 worldPos;\n"
                                          "out vec2 TexCoord;\n"
                                          "out vec3 Normal;\n"
                                          "\n"
                                          "uniform mat4 model;\n"
                                          "uniform mat3 normalMatrix;\n"
                                          "uniform mat4 view;\n"
                                          "uniform mat4 projection;\n"
                                          "\n"
                                          "void main() {\n"
                                          "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                                          "    TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
                                          "    Normal = normalMatrix * aNormal;\n"
                                          "    worldPos = vec3(model * vec4(aPos, 1.0));\n"
                                          "}";

static const char *fragmentShaderSource = "#version 430 core\n"
                                          "\n"
                                          "out vec4 FragColor;\n"
                                          "\n"
                                          "in vec3 worldPos;\n"
                                          "in vec2 TexCoord;\n"
                                          "in vec3 Normal;\n"
                                          "\n"
                                          "uniform vec4 color;\n"
                                          "\n"
                                          "\n"
                                          "uniform vec3 lightPos;\n"
                                          "uniform vec3 lightColor;\n"
                                          "\n"
                                          "void main() {\n"
                                          "\n"
                                          "    // ambient\n"
                                          "    float ambientStrength = 0.1;\n"
                                          "    vec3 ambient = ambientStrength * lightColor;\n"
                                          "\n"
                                          "    // diffuse\n"
                                          "    vec3 norm = normalize(Normal);\n"
                                          "    vec3 lightDir = normalize(lightPos - worldPos);\n"
                                          "    float diff = max(dot(norm, lightDir), 0.0);\n"
                                          "    vec3 diffuse = diff * lightColor;\n"
                                          "\n"
                                          "    vec4 result = vec4(ambient + diffuse, 1.f) * color;\n"
                                          "    \n"
                                          "    if (result.a < 0.1f) {\n"
                                          "        discard;\n"
                                          "    }\n"
                                          "    \n"
                                          "    FragColor = vec4(result);\n"
                                          "}";

static const char *texturedFragmentShaderSource = "#version 430 core\n"
                                                  "\n"
                                                  "out vec4 FragColor;\n"
                                                  "\n"
                                                  "in vec3 worldPos;\n"
                                                  "in vec2 TexCoord;\n"
                                                  "in vec3 Normal;\n"
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
                                                  "uniform vec3 lightPos;\n"
                                                  "uniform vec3 lightColor;\n"
                                                  "\n"
                                                  "void main() {\n"
                                                  "\n"
                                                  "    // ambient\n"
                                                  "    float ambientStrength = 0.1;\n"
                                                  "    vec3 ambient = ambientStrength * lightColor;\n"
                                                  "\n"
                                                  "    // diffuse\n"
                                                  "    vec3 norm = normalize(Normal);\n"
                                                  "    vec3 lightDir = normalize(lightPos - worldPos);\n"
                                                  "    float diff = max(dot(norm, lightDir), 0.0);\n"
                                                  "    vec3 diffuse = diff * lightColor;\n"
                                                  "\n"
                                                  "    vec4 sampleColor = texture(material.texture_diffuse1, TexCoord);\n"
                                                  "\n"
                                                  "    vec4 result = vec4(ambient + diffuse, 1.f) * sampleColor;\n"
                                                  "    if (result.a < 0.1f) {\n"
                                                  "        discard;\n"
                                                  "    }\n"
                                                  "    FragColor = result;\n"
                                                  "}";


bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum) {
    if (Super::init()) {
        impl->_color = Mesh::DefaultColor();
        Dispatch::async(Dispatch::Render, [_self = shared_from_this(),
                                                     vertices = std::vector<Vertex>(vertices, vertices + verticesNum),
                                                             indices = std::vector<uint32_t>(indices, indices + _indicesNum)]() mutable {

            StaticMeshNode *self = (StaticMeshNode *)_self.get();

            if (!self->impl->mesh.init(vertices.data(), vertices.size(), indices.data(), indices.size())) {

                ANLog("Mesh init fail!");
            }

            ++self->impl->ins_cnt;
        });
        return true;
    }
    return false;
}

bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum, TextureInfo *textures, uint64_t textureNum) {
    if (Super::init()) {
        Dispatch::async(Dispatch::Render, [_self = shared_from_this(),
                                                     vertices = std::vector<Vertex>(vertices, vertices + verticesNum),
                                                     indices = std::vector<uint32_t>(indices, indices + _indicesNum),
                                                     textures = std::vector<TextureInfo>(textures, textures + textureNum)]() mutable {

            StaticMeshNode *self = (StaticMeshNode *)_self.get();

            if (!self->impl->mesh.init(vertices.data(), vertices.size(), indices.data(), indices.size(), textures.data(), textures.size())) {

                ANLog("Mesh init fail!");
            }

            ++self->impl->ins_cnt;
        });
        return true;
    }
    return false;
}


void StaticMeshNode::render(const RenderContext &context) {
    Super::render(context);

    static bool isShaderInited = false;
    if (!isShaderInited) {
        isShaderInited = true;
        ANAssert(vertexLibrary.init(ShaderLibraryType::Vertex, vertexShaderSource));
        ANAssert(fragmentLibrary.init(ShaderLibraryType::Fragment, fragmentShaderSource));
        ANAssert(texturedFragmentLibrary.init(ShaderLibraryType::Fragment, texturedFragmentShaderSource));

        ANAssert(pipeline.init(vertexLibrary, fragmentLibrary));
        ANAssert(texturedPipeline.init(vertexLibrary, texturedFragmentLibrary));


        GetGame().registerCleanupTask([] {
            Dispatch::async(Dispatch::Render, [] {
                vertexLibrary.deinit();
                fragmentLibrary.deinit();
                texturedFragmentLibrary.deinit();
                pipeline.deinit();
                texturedPipeline.deinit();
            });
        });
    }

    RenderPipeline &currentPipeline = impl->mesh.isTextured() ? texturedPipeline : pipeline;

    currentPipeline.activate();

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    currentPipeline.setMat4("projection", cameraNode->getProjectionMatrix());
    currentPipeline.setMat4("view", cameraNode->getViewMatrix());
    Math::mat4 modelMatrix = getModelViewMatrix();
    currentPipeline.setMat4("model", modelMatrix);

    currentPipeline.setMat3("normalMatrix", Math::mat3(Math::transpose(Math::inverse(modelMatrix))));

    currentPipeline.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    Math::vec3 lightPos(1.2f, 1.0f, 2.0f);
    currentPipeline.setVec3("lightPos", lightPos);

    impl->mesh.render(currentPipeline);

//    shader.setVec4("color", { 0.0644f, 0.920f, 0.564f, 1.f });
//    glDrawArrays(GL_LINE_LOOP, 0, vertexNum);
}

std::shared_ptr<StaticMeshNode> StaticMeshNode::copy() {
    auto self_copy = Self::Alloc();

    /// call Super::init
    if (!self_copy->Super::init()) {
        return nullptr;
    }

    self_copy->impl = impl;
    ++impl->ins_cnt;

    return self_copy;
}

void StaticMeshNode::setColor(Math::vec4 color) {
    impl->_color = color;
    Dispatch::async(Dispatch::Render, [color, weakSelf = weak_from_this()] {
      auto _self = weakSelf.lock();
      if (_self) {
          Self *self = (Self *)_self.get();
          self->impl->mesh.setColor(color);
      }
    });
}


}