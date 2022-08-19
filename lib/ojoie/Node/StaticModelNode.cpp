//
// Created by Aleudillonam on 8/7/2022.
//

#include "Render/Model.hpp"
#include "Node/StaticModelNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"

#include "Render/Renderer.hpp"

namespace AN {

static RC::RenderPipeline pipeline;

static RC::ShaderLibrary vertexLibrary;
static RC::ShaderLibrary fragmentLibrary;

struct StaticModelNode::Impl {
    std::atomic_int ins_cnt{};
    Model model;
};

StaticModelNode::StaticModelNode() : impl(new Impl{}) {
    _needsRender = true;
}

StaticModelNode::~StaticModelNode() {
    GetRenderer().resourceFence();
    uniformBuffer.deinit();
    if (--impl->ins_cnt == 0) {
        impl->model.deinit();
        delete impl;
    }
}

struct ModelUniform {
    alignas(16) Math::mat4 model;
    alignas(16) Math::mat4 view;
    alignas(16) Math::mat4 projection;
    alignas(16) Math::mat4 normalMatrix;
};

bool StaticModelNode::init(const char *modelPath) {
    if (Super::init()) {
        if (!impl->model.init(modelPath)) {
            return false;
        }

        if (!uniformBuffer.init(sizeof(ModelUniform))) {
            return false;
        }

        ++impl->ins_cnt;

        return true;
    }

    return false;
}



void StaticModelNode::render(const RenderContext &context) {
    Super::render(context);

    static bool isShaderInited = false;
    if (!isShaderInited) {
        isShaderInited = true;
        ANAssert(vertexLibrary.initWithPath(context, RC::ShaderLibraryType::Vertex, "MeshNode.vert.spv"));
        ANAssert(fragmentLibrary.initWithPath(context, RC::ShaderLibraryType::Fragment, "MeshNodeTextured.frag.spv"));

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
        renderPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineDescriptor.colorAttachments[0].rgbBlendOperation = RC::BlendOperation::Add;

        renderPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::Zero;
        renderPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::Zero;
        renderPipelineDescriptor.colorAttachments[0].alphaBlendOperation = RC::BlendOperation::Add;


        renderPipelineDescriptor.vertexDescriptor = vertexDescriptor;
        renderPipelineDescriptor.depthStencilDescriptor = depthStencilDescriptor;

        renderPipelineDescriptor.bindings[0] = RC::BindingType::Uniform;
        renderPipelineDescriptor.bindings[1] = RC::BindingType::Uniform;
        renderPipelineDescriptor.bindings[2] = RC::BindingType::Sampler;
        renderPipelineDescriptor.bindings[3] = RC::BindingType::Texture;

        renderPipelineDescriptor.rasterSampleCount = context.msaaSamples;
        renderPipelineDescriptor.alphaToOneEnabled = false;
        renderPipelineDescriptor.alphaToCoverageEnabled = false;



        ANAssert(pipeline.init(renderPipelineDescriptor));


        GetGame().registerCleanupTask([] {
            Dispatch::async(Dispatch::Render, [] {
                vertexLibrary.deinit();
                fragmentLibrary.deinit();
                pipeline.deinit();
            });
        });
    }

    pipeline.bind();

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    ModelUniform *uniform = (ModelUniform *)uniformBuffer.content();


    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->normalMatrix = Math::mat3(Math::transpose(Math::inverse(modelMatrix)));
    uniform->view = cameraNode->getViewMatrix();
    uniform->projection = cameraNode->getProjectionMatrix();


    RC::BindUniformBuffer(0, uniformBuffer);


    impl->model.render();
}


}