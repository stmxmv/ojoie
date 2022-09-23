//
// Created by Aleudillonam on 8/7/2022.
//

#include "Render/Model.hpp"
#include "Node/StaticModelNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"

#include "Render/Renderer.hpp"

namespace AN {

static RC::RenderPipelineState pipelineState;


struct StaticModelNode::Impl {
    std::atomic_int ins_cnt{};
    Model model;

    Math::mat4 preModel{ 1.f };
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
    alignas(16) Math::mat3x4 normalMatrix;
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

        RC::RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
        if (context.forwardShading) {
            renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "MeshNode.vert.spv" };
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "MeshNodeTextured.frag.spv" };
        } else {

            if (context.antiAliasing == AntiAliasingMethod::TAA) {
                renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "geometryTAA.vert.spv" };
                renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTexturedTAA.frag.spv" };

                renderPipelineStateDescriptor.colorAttachments[2].writeMask = RC::ColorWriteMask::All;
                renderPipelineStateDescriptor.colorAttachments[2].blendingEnabled = false;
            } else {
                renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "geometry.vert.spv" };
                renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTextured.frag.spv" };
            }

            renderPipelineStateDescriptor.colorAttachments[1].writeMask = RC::ColorWriteMask::All;
            renderPipelineStateDescriptor.colorAttachments[1].blendingEnabled = true;

            renderPipelineStateDescriptor.colorAttachments[1].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
            renderPipelineStateDescriptor.colorAttachments[1].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
            renderPipelineStateDescriptor.colorAttachments[1].rgbBlendOperation = RC::BlendOperation::Add;

            renderPipelineStateDescriptor.colorAttachments[1].sourceAlphaBlendFactor = RC::BlendFactor::One;
            renderPipelineStateDescriptor.colorAttachments[1].destinationAlphaBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
            renderPipelineStateDescriptor.colorAttachments[1].alphaBlendOperation = RC::BlendOperation::Add;
        }

        renderPipelineStateDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
        renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;

        renderPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
        renderPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = RC::BlendOperation::Add;

        renderPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::One;
        renderPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = RC::BlendOperation::Add;


        renderPipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
        renderPipelineStateDescriptor.depthStencilDescriptor = depthStencilDescriptor;


        renderPipelineStateDescriptor.rasterSampleCount = context.msaaSamples;
        renderPipelineStateDescriptor.alphaToOneEnabled = false;
        renderPipelineStateDescriptor.alphaToCoverageEnabled = false;

        renderPipelineStateDescriptor.cullMode = RC::CullMode::Back;

        ANAssert(pipelineState.init(renderPipelineStateDescriptor));


        GetGame().registerCleanupTask([] {
            Dispatch::async(Dispatch::Render, [] {
                pipelineState.deinit();
            });
        });
    }

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;
    renderCommandEncoder.setRenderPipelineState(pipelineState);

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    if (context.antiAliasing == AntiAliasingMethod::TAA) {
        struct TaaUniform {
            float screenWidth, screenHeight;
            int offsetIdx;
            alignas(16) Math::mat4 preProjection;
            alignas(16) Math::mat4 preView;
            alignas(16) Math::mat4 preModel;
        } taaUniform;

        taaUniform.screenWidth = context.frameWidth;
        taaUniform.screenHeight = context.frameHeight;
        taaUniform.preProjection = cameraNode->getPreProjection();
        taaUniform.preView = cameraNode->getPreView();
        taaUniform.preModel = impl->preModel;
        taaUniform.offsetIdx = context.frameCount;

        RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(TaaUniform));
        memcpy(uniformAllocation.map(), &taaUniform, sizeof taaUniform);

        renderCommandEncoder.bindUniformBuffer(0,
                                               uniformAllocation.getOffset(),
                                               uniformAllocation.getSize(),
                                               uniformAllocation.getBuffer(), 1);
    }

    RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(ModelUniform));
    ModelUniform *uniform = (ModelUniform *)uniformAllocation.map();


    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->view = cameraNode->getViewMatrix();
    uniform->projection = cameraNode->getProjectionMatrix();
    uniform->normalMatrix = Math::transpose(Math::inverse(Math::mat3(modelMatrix)));

    impl->preModel = modelMatrix;

    renderCommandEncoder.bindUniformBuffer(0, uniformAllocation.getOffset(), uniformAllocation.getSize(), uniformAllocation.getBuffer());


    impl->model.render();
}


}