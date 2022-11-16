//
// Created by Aleudillonam on 8/3/2022.
//
#include "Node/StaticMeshNode.hpp"
#include "Core/Game.hpp"
#include "Node/CameraNode.hpp"
#include "Render/RenderPipelineState.hpp"
#include "Render/Renderer.hpp"
#include "Render/TextureLoader.hpp"
#include "Render/Scene.hpp"

#include <glad/glad.h>

namespace AN {


StaticMeshNode::StaticMeshNode() {
    _needsRender = true;
}

StaticMeshNode::~StaticMeshNode() {

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


        if (context.antiAliasing == AntiAliasingMethod::TAA) {
            renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "geometryTAA.vert.spv" };
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTAA.frag.spv" };

            renderPipelineStateDescriptor.colorAttachments[2].writeMask = RC::ColorWriteMask::All;
            renderPipelineStateDescriptor.colorAttachments[2].blendingEnabled = false;

        } else {
            renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "geometry.vert.spv" };
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometry.frag.spv" };
        }

        renderPipelineStateDescriptor.colorAttachments[1].writeMask = RC::ColorWriteMask::All;
        renderPipelineStateDescriptor.colorAttachments[1].blendingEnabled = true;

        renderPipelineStateDescriptor.colorAttachments[1].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
        renderPipelineStateDescriptor.colorAttachments[1].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineStateDescriptor.colorAttachments[1].rgbBlendOperation = RC::BlendOperation::Add;

        renderPipelineStateDescriptor.colorAttachments[1].sourceAlphaBlendFactor = RC::BlendFactor::Zero;
        renderPipelineStateDescriptor.colorAttachments[1].destinationAlphaBlendFactor = RC::BlendFactor::One;
        renderPipelineStateDescriptor.colorAttachments[1].alphaBlendOperation = RC::BlendOperation::Add;
    }

    renderPipelineStateDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
    renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;

    renderPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = RC::BlendOperation::Add;

    renderPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::Zero;
    renderPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::One;
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
        if (context.antiAliasing == AntiAliasingMethod::TAA) {
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTexturedTAA.frag.spv" };
        } else {
            renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "geometryTextured.frag.spv" };
        }
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
        _color = Mesh::DefaultColor();

        _vertices.assign(vertices, vertices + verticesNum);
        _indices.assign(indices, indices + _indicesNum);

        return true;
    }
    return false;
}

bool StaticMeshNode::init(Vertex *vertices, uint64_t verticesNum, uint32_t *indices, uint64_t _indicesNum,
                          StaticMeshNodeTextureInfo *textures, uint64_t textureNum) {
    if (Super::init()) {

        _vertices.assign(vertices, vertices + verticesNum);
        _indices.assign(indices, indices + _indicesNum);

        for (int i = 0; i < textureNum; ++i) {
            textureInfos.push_back({ textures[i].name, textures[i].type });
        }

        return true;
    }
    return false;
}



void StaticMeshNode::StaticMeshNodeSceneProxy::render(const RenderContext &context) {
    Super::SceneProxyType::render(context);

    RC::RenderPipelineState &currentPipeline = mesh.isTextured() ? texturedPipelineState : pipelineState;

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;
    renderCommandEncoder.setRenderPipelineState(currentPipeline);


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
        taaUniform.preProjection = getScene().getPreProjectionMatrix();
        taaUniform.preView = getScene().getPreViewMatrix();
        taaUniform.preModel = preModel;
        taaUniform.offsetIdx = context.frameCount;

        RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(TaaUniform));
        memcpy(uniformAllocation.map(), &taaUniform, sizeof taaUniform);

        renderCommandEncoder.bindUniformBuffer(0,
                                               uniformAllocation.getOffset(),
                                               uniformAllocation.getSize(),
                                               uniformAllocation.getBuffer(), 1);
    }

    RC::BufferAllocation uniformAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::UniformBuffer, sizeof(Uniform));
    Uniform *uniform = (Uniform *)uniformAllocation.map();

    Math::mat4 modelMatrix = getModelViewMatrix();

    uniform->model = modelMatrix;
    uniform->view = getScene().getViewMatrix();
    uniform->projection = getScene().getProjectionMatrix();
    uniform->normalMatrix = Math::transpose(Math::inverse(Math::mat3(modelMatrix)));

    preModel = modelMatrix;

    renderCommandEncoder.bindUniformBuffer(0, uniformAllocation.getOffset(), uniformAllocation.getSize(), uniformAllocation.getBuffer());

    mesh.render(context);
}

bool StaticMeshNode::StaticMeshNodeSceneProxy::createRenderResources() {
    Super::SceneProxyType::createRenderResources();
    auto meshNode = (StaticMeshNode&)(owner);

    for (auto &textureInfo : meshNode.textureInfos) {
        RC::Texture texture = TextureLoader::loadTexture(textureInfo.name.c_str());
        textures.push_back(std::move(texture));
    }

    if (textures.empty()) {
        if (!mesh.init(meshNode._vertices.data(), meshNode._vertices.size(), meshNode._indices.data(), meshNode._indices.size())) {
            return false;
        }
    } else {

        std::vector<TextureInfo> textureInfos;
        for (int i = 0; i < textures.size(); ++i) {
            textureInfos.push_back({ &textures[i], meshNode.textureInfos[i].type });
        }

        if (!mesh.init(meshNode._vertices.data(), meshNode._vertices.size(), meshNode._indices.data(), meshNode._indices.size(),
                       textureInfos.data(), textureInfos.size())) {
            return false;
        }

    }



    initPipelineState();

    return true;
}

void StaticMeshNode::StaticMeshNodeSceneProxy::destroyRenderResources() {
    Super::SceneProxyType::destroyRenderResources();

    mesh.deinit();

    textures.clear();
}

}