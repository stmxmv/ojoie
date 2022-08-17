//
// Created by Aleudillonam on 8/3/2022.
//
#include "Node/StaticMeshNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"
#include "Render/RenderPipeline.hpp"
#include "Render/Renderer.hpp"



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
    GetRenderQueue().enqueue([uniform = std::move(uniformBuffer)]() mutable {
        GetRenderer().resourceFence();
        uniform.deinit();
    });

    if (--impl->ins_cnt == 0) {
        GetRenderQueue().enqueue([impl = impl]() mutable {
            GetRenderer().resourceFence();
            delete impl;
        });
    }
}

static RC::ShaderLibrary vertexLibrary;
static RC::ShaderLibrary fragmentLibrary;
static RC::ShaderLibrary texturedFragmentLibrary;
static RC::RenderPipeline pipeline;
static RC::RenderPipeline texturedPipeline;


struct Uniform {
    alignas(16) Math::mat4 model;
    alignas(16) Math::mat4 view;
    alignas(16) Math::mat4 projection;
    alignas(16) Math::mat4 normalMatrix;
};


bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum) {
    if (Super::init()) {
        impl->_color = Mesh::DefaultColor();
        Dispatch::async(Dispatch::Render, [_self = shared_from_this(),
                                                     vertices = std::vector<Vertex>(vertices, vertices + verticesNum),
                                                             indices = std::vector<uint32_t>(indices, indices + _indicesNum)]() mutable {

            StaticMeshNode *self = (StaticMeshNode *)_self.get();

            if (!self->impl->mesh.init(vertices.data(), vertices.size(), indices.data(), indices.size())) {
                return;
            }

            if (!self->uniformBuffer.init(sizeof(Uniform))) {
                return;
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
                return;
            }

            if (!self->uniformBuffer.init(sizeof(Uniform))) {
                return;
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
        ANAssert(vertexLibrary.initWithPath(context, RC::ShaderLibraryType::Vertex, "MeshNode.vert.spv"));
        ANAssert(fragmentLibrary.initWithPath(context, RC::ShaderLibraryType::Fragment, "MeshNode.frag.spv"));
        ANAssert(texturedFragmentLibrary.initWithPath(context, RC::ShaderLibraryType::Fragment, "MeshNodeTextured.frag.spv"));

        RC::VertexDescriptor vertexDescriptor{};
        vertexDescriptor.attributes[0].format = RC::VertexFormat::Float3;
        vertexDescriptor.attributes[0].binding = 0;
        vertexDescriptor.attributes[0].offset = 0;

        vertexDescriptor.attributes[1].format = RC::VertexFormat::Float3;
        vertexDescriptor.attributes[1].binding = 0;
        vertexDescriptor.attributes[1].offset = offsetof(Vertex, normal);

        vertexDescriptor.attributes[2].format = RC::VertexFormat::Float2;
        vertexDescriptor.attributes[2].binding = 0;
        vertexDescriptor.attributes[2].offset = offsetof(Vertex, texCoord);

        vertexDescriptor.layouts[0].stepFunction = RC::VertexStepFunction::PerVertex;
        vertexDescriptor.layouts[0].stride = sizeof(Vertex);

        RC::DepthStencilDescriptor depthStencilDescriptor{};
        depthStencilDescriptor.depthTestEnabled = true;
        depthStencilDescriptor.depthWriteEnabled = true;
        depthStencilDescriptor.depthCompareFunction = RC::CompareFunction::Less;

        RC::RenderPipelineDescriptor renderPipelineDescriptor{};
        renderPipelineDescriptor.vertexFunction = { .name = "main", .library = &vertexLibrary };
        renderPipelineDescriptor.fragmentFunction = { .name = "main", .library = &fragmentLibrary };

        renderPipelineDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
        renderPipelineDescriptor.colorAttachments[0].blendingEnabled = true;

        renderPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
        renderPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = RC::BlendFactor::OneMinusDestinationAlpha;
        renderPipelineDescriptor.colorAttachments[0].rgbBlendOperation = RC::BlendOperation::Add;

        renderPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::One;
        renderPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::Zero;
        renderPipelineDescriptor.colorAttachments[0].alphaBlendOperation = RC::BlendOperation::Add;


        renderPipelineDescriptor.vertexDescriptor = vertexDescriptor;
        renderPipelineDescriptor.depthStencilDescriptor = depthStencilDescriptor;

        renderPipelineDescriptor.bindings[0] = RC::BindingType::Uniform;
        renderPipelineDescriptor.bindings[1] = RC::BindingType::Uniform;


        ANAssert(pipeline.init(renderPipelineDescriptor));

        renderPipelineDescriptor.fragmentFunction = { .name = "main", .library = &texturedFragmentLibrary };

        renderPipelineDescriptor.bindings[2] = RC::BindingType::Sampler;
        renderPipelineDescriptor.bindings[3] = RC::BindingType::Texture;

        ANAssert(texturedPipeline.init(renderPipelineDescriptor));


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

    RC::RenderPipeline &currentPipeline = impl->mesh.isTextured() ? texturedPipeline : pipeline;

    currentPipeline.bind();

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    Uniform *uniform = (Uniform *)uniformBuffer.mapMemory();


    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->normalMatrix = Math::mat3(Math::transpose(Math::inverse(modelMatrix)));
    uniform->view = cameraNode->getViewMatrix();
    uniform->projection = cameraNode->getProjectionMatrix();


    uniformBuffer.unMapMemory();

    RC::BindUniformBuffer(0, uniformBuffer);

    impl->mesh.render(context, currentPipeline);

}

std::shared_ptr<StaticMeshNode> StaticMeshNode::copy() {
    auto self_copy = Self::Alloc();

    /// call Super::init
    if (!self_copy->Super::init()) {
        return nullptr;
    }

    self_copy->impl = impl;

    GetRenderQueue().enqueue([weakClone = self_copy->weak_from_this(), weakSelf = weak_from_this()] {
        auto _clone = weakClone.lock();
        auto _self = weakSelf.lock();
        if (_clone && _self) {
            Self *clone = (Self *)_clone.get();
            Self *self = (Self *)_self.get();
            clone->uniformBuffer = self->uniformBuffer.copy();
        }
    });

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