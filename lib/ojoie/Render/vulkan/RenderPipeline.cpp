//
// Created by Aleudillonam on 8/13/2022.
//

#include "Render/private/vulkan/RenderPipeline.hpp"
#include "Render/RenderContext.hpp"
#include "Render/ShaderFunction.hpp"
#include "Render/Texture.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/Device.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

namespace AN {


std::string chooseShaderPath(const char *path) {
    static const char *possiblePathPrefix[] = {"./Resources/Shaders", ".", "./Shaders"};
    std::vector<char>  code;
    for (const char *prefix : possiblePathPrefix) {
        auto realPath = std::format("{}/{}", prefix, path);
        if (std::filesystem::exists(realPath)) {
            return realPath;
        }
    }
    return {};
}

static std::vector<VkVertexInputBindingDescription> getVertexInputBindingDescription(
        const VK::RenderPipelineDescriptor &descriptor) {

    std::vector<VkVertexInputBindingDescription> vertexInputBindings;
    vertexInputBindings.reserve(descriptor.vertexDescriptor->layouts.size());

    for (uint32_t i = 0; i < descriptor.vertexDescriptor->layouts.size(); ++i) {
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = i;
        vertexInputBindingDescription.stride = descriptor.vertexDescriptor->layouts[i].stride;
        VkVertexInputRate inputRate;

        switch (descriptor.vertexDescriptor->layouts[i].stepFunction) {
            case kVertexStepFunctionPerVertex:
                inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                break;
            case kVertexStepFunctionPerInstance:
                inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                break;
        }

        vertexInputBindingDescription.inputRate = inputRate;

        vertexInputBindings.push_back(vertexInputBindingDescription);
    }
    return vertexInputBindings;
}

static std::vector<VkVertexInputAttributeDescription>
getVertexInputAttributeDescription(const VK::RenderPipelineDescriptor &descriptor) {
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    vertexAttributes.reserve(descriptor.vertexDescriptor->attributes.size());
    for (const auto &attribute : descriptor.vertexDescriptor->attributes) {
        VkVertexInputAttributeDescription description{};
        description.location = attribute.location;
        description.offset   = attribute.offset;
        description.binding  = attribute.binding;

        int                 dimension = attribute.dimension;
        VertexChannelFormat format    = attribute.format;
        ANAssert(format < kChannelFormatCount);
        switch (format) {
            case kChannelFormatColor:
                goto __color;
            case kChannelFormatFloat:
                switch (dimension) {
                    case 1:
                        description.format = VK_FORMAT_R32_SFLOAT;
                        break;
                    case 2:
                        description.format = VK_FORMAT_R32G32_SFLOAT;
                        break;
                    case 3:
                        description.format = VK_FORMAT_R32G32B32_SFLOAT;
                        break;
                    case 4:
                    __color:
                        description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        break;
                    default:
                        AN_LOG(Error, "not support vertex channel dimension %d", dimension);
                }
                break;
            case kChannelFormatByte:
                switch (dimension) {
                    case 1:
                        description.format = VK_FORMAT_R8_UNORM;
                        break;
                    case 2:
                        description.format = VK_FORMAT_R8G8_UNORM;
                        break;
                    case 3:
                        description.format = VK_FORMAT_R8G8B8_UNORM;
                        break;
                    case 4:
                        description.format = VK_FORMAT_R8G8B8A8_UNORM;
                        break;
                    default:
                        AN_LOG(Error, "not support vertex channel dimension %d", dimension);
                }
                break;

            default:
                AN_LOG(Error, "%s", "not support channel format");
        }
        vertexAttributes.push_back(description);
    }
    return vertexAttributes;
}

static VkBlendFactor toVkBlendFactor(BlendFactor blendFactor) {
    ANAssert(blendFactor < kBlendFactorCount);
    switch (blendFactor) {
        case kBlendFactorZero:
            return VK_BLEND_FACTOR_ZERO;
        case kBlendFactorOne:
            return VK_BLEND_FACTOR_ONE;
        case kBlendFactorSourceColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case kBlendFactorOneMinusSourceColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case kBlendFactorDestinationColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case kBlendFactorOneMinusDestinationColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case kBlendFactorSourceAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case kBlendFactorOneMinusSourceAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case kBlendFactorDestinationAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case kBlendFactorOneMinusDestinationAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case kBlendFactorBlendColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case kBlendFactorOneMinusBlendColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case kBlendFactorBlendAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case kBlendFactorOneMinusBlendAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case kBlendFactorSourceAlphaSaturated:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case kBlendFactorSource1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case kBlendFactorOneMinusSource1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case kBlendFactorSource1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case kBlendFactorOneMinusSource1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            throw Exception("Invalid enum value");
    }
}

static VkBlendOp toVkBlendOp(BlendOperation operation) {
    switch (operation) {
        case kBlendOperationAdd:
            return VK_BLEND_OP_ADD;
        case kBlendOperationSubtract:
            return VK_BLEND_OP_SUBTRACT;
        case kBlendOperationReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case kBlendOperationMin:
            return VK_BLEND_OP_MIN;
        case kBlendOperationMax:
            return VK_BLEND_OP_MAX;
        default:
            throw Exception("Invalid enum value");
    }
}

static VkColorComponentFlags toVkColorComponentFlags(ColorWriteMask writeMask) {
    VkColorComponentFlags flags = 0;
    if (writeMask == kColorWriteMaskAll) {
        flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        return flags;
    }

    if (writeMask & kColorWriteMaskRed) {
        flags |= VK_COLOR_COMPONENT_R_BIT;
    }
    if (writeMask & kColorWriteMaskGreen) {
        flags |= VK_COLOR_COMPONENT_G_BIT;
    }
    if (writeMask & kColorWriteMaskBlue) {
        flags |= VK_COLOR_COMPONENT_B_BIT;
    }
    if (writeMask & kColorWriteMaskAlpha) {
        flags |= VK_COLOR_COMPONENT_A_BIT;
    }

    return flags;
}

static VkCompareOp toVkCompareOp(CompareFunction function) {
    switch (function) {
        case kCompareFunctionNever:
            return VK_COMPARE_OP_NEVER;
        case kCompareFunctionLess:
            return VK_COMPARE_OP_LESS;
        case kCompareFunctionEqual:
            return VK_COMPARE_OP_EQUAL;
        case kCompareFunctionLEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case kCompareFunctionGreater:
            return VK_COMPARE_OP_GREATER;
        case kCompareFunctionNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case kCompareFunctionGEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case kCompareFunctionAlways:
            return VK_COMPARE_OP_ALWAYS;
        default:
            throw Exception("Invalid enum value");
    }
}


static VkShaderStageFlags toVkPipelineStageFlags(ShaderStageFlags stageFlag) {
    VkShaderStageFlags ret{};
    if (stageFlag & kShaderStageVertex) {
        ret |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (stageFlag & kShaderStageFragment) {
        ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (stageFlag & kShaderStageGeometry) {
        ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    return ret;
}

inline static VkCullModeFlags toVkCullMode(CullMode cullMode) {
    VkCullModeFlags ret{};

    if (cullMode & kCullModeFront) {
        ret |= VK_CULL_MODE_FRONT_BIT;
    }

    if (cullMode & kCullModeBack) {
        ret |= VK_CULL_MODE_BACK_BIT;
    }

    return ret;
}


}// namespace AN

namespace AN::VK {

bool RenderPipeline::init(AN::VK::Device                 &device,
                          const RenderPipelineDescriptor &descriptor,
                          PipelineLayout                 &pipelineLayout,
                          RenderPass                     &renderPass,
                          VkPipelineCache                 pipelineCache) {
    _device         = &device;
    _pipelineLayout = &pipelineLayout;

    const RenderPipelineStateDescriptor &stateDescriptor = *descriptor.stateDescriptor;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = (VkShaderModule) stateDescriptor.vertexFunction.code;
    vertShaderStageInfo.pName  = stateDescriptor.vertexFunction.entry;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = (VkShaderModule) stateDescriptor.fragmentFunction.code;
    fragShaderStageInfo.pName  = stateDescriptor.fragmentFunction.entry;


    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


    std::vector<VkVertexInputBindingDescription>   vertexInputBindingDescriptions   = getVertexInputBindingDescription(descriptor);
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions = getVertexInputAttributeDescription(descriptor);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


    vertexInputInfo.vertexBindingDescriptionCount   = vertexInputBindingDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions      = vertexInputBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions    = vertexInputAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    VkDynamicState                   dynamicStateEnables[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t) std::size(dynamicStateEnables);
    dynamicState.pDynamicStates    = dynamicStateEnables;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = nullptr;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = AN::toVkCullMode(stateDescriptor.cullMode);
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_TRUE;// enable sample shading in the pipeline
    multisampling.minSampleShading      = 0.4f;   // min fraction for sample shading; closer to one is smooth
    multisampling.rasterizationSamples  = (VkSampleCountFlagBits) stateDescriptor.rasterSampleCount;
    multisampling.pSampleMask           = nullptr;
    multisampling.alphaToCoverageEnable = stateDescriptor.alphaToCoverageEnabled;
    multisampling.alphaToOneEnable      = stateDescriptor.alphaToOneEnabled;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(stateDescriptor.colorAttachments.size());
    for (uint32_t i = 0; i < colorBlendAttachments.size(); ++i) {
        colorBlendAttachments[i].colorWriteMask = toVkColorComponentFlags(stateDescriptor.colorAttachments[i].writeMask);

        colorBlendAttachments[i].blendEnable = stateDescriptor.colorAttachments[i].blendingEnabled;

        colorBlendAttachments[i].srcColorBlendFactor = toVkBlendFactor(stateDescriptor.colorAttachments[i].sourceRGBBlendFactor);
        colorBlendAttachments[i].dstColorBlendFactor = toVkBlendFactor(stateDescriptor.colorAttachments[i].destinationRGBBlendFactor);
        colorBlendAttachments[i].colorBlendOp        = toVkBlendOp(stateDescriptor.colorAttachments[i].rgbBlendOperation);

        colorBlendAttachments[i].srcAlphaBlendFactor = toVkBlendFactor(stateDescriptor.colorAttachments[i].sourceAlphaBlendFactor);
        colorBlendAttachments[i].dstAlphaBlendFactor = toVkBlendFactor(stateDescriptor.colorAttachments[i].destinationAlphaBlendFactor);
        colorBlendAttachments[i].alphaBlendOp        = toVkBlendOp(stateDescriptor.colorAttachments[i].alphaBlendOperation);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = colorBlendAttachments.size();
    colorBlending.pAttachments      = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;


    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = stateDescriptor.depthStencilDescriptor.depthTestEnabled;
    depthStencil.depthWriteEnable      = stateDescriptor.depthStencilDescriptor.depthWriteEnabled;
    depthStencil.depthCompareOp        = toVkCompareOp(stateDescriptor.depthStencilDescriptor.depthCompareFunction);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.0f;// Optional
    depthStencil.maxDepthBounds        = 1.0f;// Optional
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {};// Optional
    depthStencil.back                  = {};// Optional

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (uint32_t) std::size(shaderStages);
    pipelineInfo.pStages    = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = pipelineLayout.vkPipelineLayout();
    pipelineInfo.renderPass          = renderPass.vkRenderPass();
    pipelineInfo.subpass             = descriptor.subpassIndex;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device.vkDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
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

}// namespace AN::VK