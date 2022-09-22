//
// Created by Aleudillonam on 8/13/2022.
//

#include "Render/RenderPipelineState.hpp"
#include "Render/private/vulkan/RenderPipeline.hpp"
#include "Render/private/vulkan/RenderCommandEncoder.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/RenderContext.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/Renderer.hpp"
#include "Render/Sampler.hpp"
#include "Render/Texture.hpp"

#include <fstream>
#include <vector>
#include <filesystem>
#include <format>

namespace AN::RC {


std::string chooseShaderPath(const char *path) {
    static const char *possiblePathPrefix[] = { "./Resources/Shaders", ".", "./Shaders"  };
    std::vector<char> code;
    for (const char *prefix : possiblePathPrefix) {
        auto realPath = std::format("{}/{}", prefix, path);
        if (std::filesystem::exists(realPath)) {
            return realPath;
        }
    }
    return {};
}

static std::vector<VkVertexInputBindingDescription> getVertexInputBindingDescription(const RenderPipelineStateDescriptor &descriptor) {

    std::vector<VkVertexInputBindingDescription> vertexInputBindings;
    vertexInputBindings.reserve(descriptor.vertexDescriptor.layouts.count());

    for (uint32_t i = 0; i < descriptor.vertexDescriptor.layouts.count(); ++i) {
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = i;
        vertexInputBindingDescription.stride = descriptor.vertexDescriptor.layouts[i].stride;
        VkVertexInputRate inputRate;

        switch (descriptor.vertexDescriptor.layouts[i].stepFunction) {
            case VertexStepFunction::PerVertex:
                inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                break;
            case VertexStepFunction::PerInstance:
                inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                break;
        }

        vertexInputBindingDescription.inputRate = inputRate;

        vertexInputBindings.push_back(vertexInputBindingDescription);
    }
    return vertexInputBindings;
}

static std::vector<VkVertexInputAttributeDescription>
getVertexInputAttributeDescription(const RenderPipelineStateDescriptor &descriptor) {
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    for (uint32_t i = 0; i < descriptor.vertexDescriptor.attributes.count(); ++i) {
        VkVertexInputAttributeDescription description{};
        description.location = descriptor.vertexDescriptor.attributes[i].location;
        description.offset = descriptor.vertexDescriptor.attributes[i].offset;
        description.binding = descriptor.vertexDescriptor.attributes[i].binding;
        switch (descriptor.vertexDescriptor.attributes[i].format) {
            case VertexFormat::Float2:
                description.format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case VertexFormat::Float3:
                description.format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case VertexFormat::Float4:
                description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            case VertexFormat::UChar4:
                description.format = VK_FORMAT_R8G8B8A8_UNORM;
                /// TODO
        }
        vertexAttributes.push_back(description);
    }
    return vertexAttributes;
}

static VkBlendFactor toVkBlendFactor(BlendFactor blendFactor) {
    switch (blendFactor) {
        case BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SourceColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSourceColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DestinationColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDestinationColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::SourceAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSourceAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DestinationAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDestinationAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor::BlendColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendFactor::OneMinusBlendColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::BlendAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case BlendFactor::OneMinusBlendAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SourceAlphaSaturated:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case BlendFactor::Source1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case BlendFactor::OneMinusSource1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case BlendFactor::Source1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case BlendFactor::OneMinusSource1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    throw Exception("Invalid enum value");
}

static VkBlendOp toVkBlendOp(BlendOperation operation) {
    switch (operation) {
        case BlendOperation::Add:
            return VK_BLEND_OP_ADD;
        case BlendOperation::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOperation::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOperation::Min:
            return VK_BLEND_OP_MIN;
        case BlendOperation::Max:
            return VK_BLEND_OP_MAX;
    }
    throw Exception("Invalid enum value");
}

static VkColorComponentFlags toVkColorComponentFlags(ColorWriteMask writeMask) {
    VkColorComponentFlags flags = 0;
    if (writeMask == ColorWriteMask::All) {
        flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        return flags;
    }

    if ((writeMask & ColorWriteMask::Red) != ColorWriteMask::None) {
        flags |= VK_COLOR_COMPONENT_R_BIT;
    }
    if ((writeMask & ColorWriteMask::Green) != ColorWriteMask::None) {
        flags |= VK_COLOR_COMPONENT_G_BIT;
    }
    if ((writeMask & ColorWriteMask::Blue) != ColorWriteMask::None) {
        flags |= VK_COLOR_COMPONENT_B_BIT;
    }
    if ((writeMask & ColorWriteMask::Alpha) != ColorWriteMask::None) {
        flags |= VK_COLOR_COMPONENT_A_BIT;
    }

    return flags;
}

static VkCompareOp toVkCompareOp(CompareFunction function) {
    switch (function) {
        case CompareFunction::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareFunction::Less:
            return VK_COMPARE_OP_LESS;
        case CompareFunction::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareFunction::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareFunction::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareFunction::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareFunction::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareFunction::Always:
            return VK_COMPARE_OP_ALWAYS;
    }
    throw Exception("Invalid enum value");
}


static VkShaderStageFlags toVkPipelineStageFlags(ShaderStageFlag stageFlag) {
    VkShaderStageFlags ret{};
    if ((stageFlag & ShaderStageFlag::Vertex) != ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if ((stageFlag & ShaderStageFlag::Fragment) != ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if ((stageFlag & ShaderStageFlag::Geometry) != ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    return ret;
}

inline static VkCullModeFlags toVkCullMode(RC::CullMode cullMode) {
    VkCullModeFlags ret{};

    if ((cullMode & RC::CullMode::Front) != RC::CullMode::None) {
        ret |= VK_CULL_MODE_FRONT_BIT;
    }

    if ((cullMode & RC::CullMode::Back) != RC::CullMode::None) {
        ret |= VK_CULL_MODE_BACK_BIT;
    }

    return ret;
}


}

namespace AN::VK {

bool RenderPipeline::init(AN::VK::Device &device, RC::RenderPipelineStateDescriptor &descriptor, RenderPass &renderPass) {
    _device = &device;

    std::string vertexLibraryPath = RC::chooseShaderPath(descriptor.vertexFunction.library);
    std::string fragmentLibraryPath = RC::chooseShaderPath(descriptor.fragmentFunction.library);

    ShaderLibrary &vertexLibrary = device.getRenderResourceCache().newShaderLibrary(vertexLibraryPath.c_str());
    ShaderLibrary &fragmentLibrary = device.getRenderResourceCache().newShaderLibrary(fragmentLibraryPath.c_str());

    ShaderFunction functions[2] = {};
    functions[0] = device.getRenderResourceCache()
                           .newShaderFunction(vertexLibrary, VK_SHADER_STAGE_VERTEX_BIT, descriptor.vertexFunction.name);
    functions[1] = device.getRenderResourceCache()
                           .newShaderFunction(fragmentLibrary, VK_SHADER_STAGE_FRAGMENT_BIT, descriptor.fragmentFunction.name);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexLibrary.vkShaderModule();
    vertShaderStageInfo.pName = descriptor.vertexFunction.name;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentLibrary.vkShaderModule();
    fragShaderStageInfo.pName = descriptor.fragmentFunction.name;


    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    std::vector<VkVertexInputBindingDescription>  vertexInputBindingDescriptions = getVertexInputBindingDescription(descriptor);
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions = getVertexInputAttributeDescription(descriptor);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


    vertexInputInfo.vertexBindingDescriptionCount = vertexInputBindingDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)std::size(dynamicStateEnables);
    dynamicState.pDynamicStates = dynamicStateEnables;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = RC::toVkCullMode(descriptor.cullMode);
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
    multisampling.minSampleShading = 0.4f; // min fraction for sample shading; closer to one is smooth
    multisampling.rasterizationSamples = (VkSampleCountFlagBits)descriptor.rasterSampleCount;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = descriptor.alphaToCoverageEnabled;
    multisampling.alphaToOneEnable = descriptor.alphaToOneEnabled;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(descriptor.colorAttachments.count());
    for (uint32_t i = 0; i < colorBlendAttachments.size(); ++i) {
        colorBlendAttachments[i].colorWriteMask = toVkColorComponentFlags(descriptor.colorAttachments[i].writeMask);

        colorBlendAttachments[i].blendEnable = descriptor.colorAttachments[i].blendingEnabled;

        colorBlendAttachments[i].srcColorBlendFactor = toVkBlendFactor(descriptor.colorAttachments[i].sourceRGBBlendFactor);
        colorBlendAttachments[i].dstColorBlendFactor = toVkBlendFactor(descriptor.colorAttachments[i].destinationRGBBlendFactor);
        colorBlendAttachments[i].colorBlendOp = toVkBlendOp(descriptor.colorAttachments[i].rgbBlendOperation);

        colorBlendAttachments[i].srcAlphaBlendFactor = toVkBlendFactor(descriptor.colorAttachments[i].sourceAlphaBlendFactor);
        colorBlendAttachments[i].dstAlphaBlendFactor = toVkBlendFactor(descriptor.colorAttachments[i].destinationAlphaBlendFactor);
        colorBlendAttachments[i].alphaBlendOp = toVkBlendOp(descriptor.colorAttachments[i].alphaBlendOperation);

    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;


    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = descriptor.depthStencilDescriptor.depthTestEnabled;
    depthStencil.depthWriteEnable = descriptor.depthStencilDescriptor.depthWriteEnabled;
    depthStencil.depthCompareOp = toVkCompareOp(descriptor.depthStencilDescriptor.depthCompareFunction);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    PipelineLayout &pipelineLayout = device.getRenderResourceCache().newPipelineLayout(functions, true);
    _pipelineLayout = &pipelineLayout;


    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (uint32_t)std::size(shaderStages);
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout.vkPipelineLayout();
    pipelineInfo.renderPass = renderPass.vkRenderPass();
    pipelineInfo.subpass = descriptor.subpassIndex;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device.vkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        ANLog("failed to create graphics pipeline!");
        return false;
    }

    return true;
}

void RenderPipeline::deinit() {
    if (graphicsPipeline) {
        vkDestroyPipeline(_device->vkDevice(), graphicsPipeline, nullptr);
        graphicsPipeline = nullptr;
    }
}

}