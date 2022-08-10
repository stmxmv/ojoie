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
                                   "//uniform sampler2D ourTexture;\n"
                                   "\n"
                                   "uniform vec4 color;\n"
                                   "\n"
                                   "void main() {\n"
                                   "//    FragColor = texture(ourTexture, TexCoord);\n"
                                   "    FragColor = vec4(color);\n"
                                   "}";

static const char *texturedFragmentShaderSource = "#version 430 core\n"
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

bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum, Texture *textures, uint64_t textureNum) {
    if (Super::init()) {
        Dispatch::async(Dispatch::Render, [_self = shared_from_this(),
                                                     vertices = std::vector<Vertex>(vertices, vertices + verticesNum),
                                                     indices = std::vector<uint32_t>(indices, indices + _indicesNum),
                                                     textures = std::vector<Texture>(textures, textures + textureNum)]() mutable {

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
    currentPipeline.setMat4("model", getModelViewMatrix());

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