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

    if (--impl->ins_cnt == 0) {
        GetRenderQueue().enqueue([impl = impl]() mutable {
            GetRenderer().resourceFence();
            impl->model.deinit();
            delete impl;
        });

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
        bool success = true;
        TaskFence fence;
        GetRenderQueue().enqueue([&fence, modelPath, &success, this] {
            if (!impl->model.init(modelPath)) {
                success = false;
            }

            ++impl->ins_cnt;

            fence.signal();
        });

        fence.wait();

        return success;
    }

    return false;
}



void StaticModelNode::render(const RenderContext &context) {
    Super::render(context);

    static bool isShaderInited = false;
    if (!isShaderInited) {
        isShaderInited = true;
        ANAssert(vertexLibrary.init(RC::ShaderLibraryType::Vertex, "MeshNode.vert.spv"));
        ANAssert(fragmentLibrary.init(RC::ShaderLibraryType::Fragment, "MeshNodeTextured.frag.spv"));

        RC::VertexDescriptor vertexDescriptor{};
        vertexDescriptor.attributes[0].format = RC::VertexFormat::Float3;
        vertexDescriptor.attributes[0].binding = 0;
        vertexDescriptor.attributes[0].offset = 0;
        vertexDescriptor.attributes[0].location = 0;

        vertexDescriptor.attributes[1].format = RC::VertexFormat::Float3;
        vertexDescriptor.attributes[1].binding = 0;
        vertexDescriptor.attributes[1].offset = offsetof(Vertex, normal);
        vertexDescriptor.attributes[1].location = 1;

        vertexDescriptor.attributes[2].format = RC::VertexFormat::Float2;
        vertexDescriptor.attributes[2].binding = 0;
        vertexDescriptor.attributes[2].offset = offsetof(Vertex, texCoord);
        vertexDescriptor.attributes[2].location = 2;

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

        renderPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::One;
        renderPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
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

        renderPipelineDescriptor.cullMode = RC::CullMode::Back;

        ANAssert(pipeline.init(renderPipelineDescriptor));


        GetGame().registerCleanupTask([] {
            Dispatch::async(Dispatch::Render, [] {
                vertexLibrary.deinit();
                fragmentLibrary.deinit();
                pipeline.deinit();
            });
        });
    }

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;
    renderCommandEncoder.bindRenderPipeline(pipeline);

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(ModelUniform));
    ModelUniform *uniform = (ModelUniform *)uniformAllocation.map();


    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->view = cameraNode->getViewMatrix();
    uniform->projection = cameraNode->getProjectionMatrix();
    uniform->normalMatrix = Math::mat3(Math::transpose(Math::inverse(modelMatrix)));


    renderCommandEncoder.bindUniformBuffer(0, uniformAllocation.getOffset(), uniformAllocation.getSize(), uniformAllocation.getBuffer());


    impl->model.render();
}


}