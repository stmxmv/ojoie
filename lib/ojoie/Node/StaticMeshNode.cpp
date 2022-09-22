//
// Created by Aleudillonam on 8/3/2022.
//
#include "Node/StaticMeshNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"
#include "Render/RenderPipelineState.hpp"
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

    if (--impl->ins_cnt == 0) {
        GetRenderQueue().enqueue([impl = impl]() mutable {
            GetRenderer().resourceFence();
            impl->mesh.deinit();
            delete impl;
        });

    }
}

static RC::RenderPipelineState pipelineState;
static RC::RenderPipelineState texturedPipelineState;

static void initPipelineState() {
    static bool isShaderInited = false;
    if (isShaderInited) {
        return;
    }

    const RenderContext &context = GetRenderer().getRenderContext();

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
        renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "MeshNode.frag.spv" };
    } else {
        renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "geometry.vert.spv" };
        renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometry.frag.spv" };

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

    if (context.forwardShading) {
        renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "MeshNodeTextured.frag.spv" };
    } else {
        renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTextured.frag.spv" };
    }



    ANAssert(texturedPipelineState.init(renderPipelineStateDescriptor));


    GetGame().registerCleanupTask([] {
        Dispatch::async(Dispatch::Render, [] {
            pipelineState.deinit();
            texturedPipelineState.deinit();
        });
    });
}

struct Uniform {
    alignas(16) Math::mat4 model;
    alignas(16) Math::mat4 view;
    alignas(16) Math::mat4 projection;
    alignas(16) Math::mat3x4 normalMatrix;
};


bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum) {
    if (Super::init()) {
        impl->_color = Mesh::DefaultColor();

        bool success = true;
        TaskFence fence;
        GetRenderQueue().enqueue([=, &success, &fence, this] {
            if (!impl->mesh.init(vertices, verticesNum, indices, _indicesNum)) {
                success = false;
            }

            ++impl->ins_cnt;
            fence.signal();
        });

        initPipelineState();

        fence.wait();

        return success;
    }
    return false;
}

bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum, TextureInfo *textures, uint64_t textureNum) {
    if (Super::init()) {

        bool success = true;
        TaskFence fence;
        GetRenderQueue().enqueue([=, &success, &fence, this] {
            if (!impl->mesh.init(vertices, verticesNum, indices, _indicesNum, textures, textureNum)) {
                success = false;
            }

            ++impl->ins_cnt;
            fence.signal();
        });

        initPipelineState();

        fence.wait();

        return success;
    }
    return false;
}


void StaticMeshNode::render(const RenderContext &context) {
    Super::render(context);

    RC::RenderPipelineState &currentPipeline = impl->mesh.isTextured() ? texturedPipelineState : pipelineState;

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;
    renderCommandEncoder.setRenderPipelineState(currentPipeline);

    auto cameraNode = GetCurrentCamera();

    if (!cameraNode) {
        return;
    }

    RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(Uniform));
    Uniform *uniform = (Uniform *)uniformAllocation.map();


    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->view = cameraNode->getViewMatrix();
    uniform->projection = cameraNode->getProjectionMatrix();
    uniform->normalMatrix = Math::transpose(Math::inverse(Math::mat3(modelMatrix)));


    renderCommandEncoder.bindUniformBuffer(0, uniformAllocation.getOffset(), uniformAllocation.getSize(), uniformAllocation.getBuffer());

    impl->mesh.render(context);

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