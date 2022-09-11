//
// Created by Aleudillonam on 8/13/2022.
//

#include "Render/RenderPipeline.hpp"
#include "Render/private/vulkan/RenderPipeline.hpp"
#include "Render/private/vulkan/RenderCommandEncoder.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/RenderContext.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/UniformBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/Sampler.hpp"
#include "Render/Texture.hpp"

#include <fstream>
#include <vector>
#include <filesystem>
#include <format>

namespace AN::RC {

static std::vector<char> readFile(const char *filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        ANLog("failed to open file!");
        return {};
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), (long long)fileSize);

    file.close();

    return buffer;
}

struct ShaderLibrary::Impl {
    VkShaderModule module;
    VK::Device *device;
};

ShaderLibrary::ShaderLibrary() : impl(new Impl{}) {}

ShaderLibrary::~ShaderLibrary() {
    deinit();
    delete impl;
}

bool ShaderLibrary::init(ShaderLibraryType type, const char *path) {
    static const char *possiblePathPrefix[] = { "./Resources/Shaders", ".", "./Shaders"  };

    std::vector<char> code;
    for (const char *prefix : possiblePathPrefix) {
        auto realPath = std::format("{}/{}", prefix, path);
        if (std::filesystem::exists(realPath)) {
            code = readFile(realPath.c_str());
            if (!code.empty()) {
                break;
            }
        }
    }

    if (code.empty()) {
        ANLog("shader library not found or empry");
        return false;
    }

    const AN::RenderContext &context = GetRenderer().getRenderContext();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.graphicContext->device->vkDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        ANLog("failed to create shader module!");
        return false;
    }

    impl->device = context.graphicContext->device;
    impl->module = shaderModule;
    _type = type;

    return true;
}

void ShaderLibrary::deinit() {
    if (impl && impl->device && impl->module) {
        vkDestroyShaderModule(impl->device->vkDevice(), impl->module, nullptr);
        impl->module = nullptr;
    }
}


struct binding_info {

    BindingType type;

    union {
        UniformBuffer *uniformBuffer;
        Sampler *sampler;
        Texture *texture;
    };

    bool operator == (const binding_info &other) const {
        if (type != other.type) { return false; }

        switch (type) {
            case BindingType::Uniform:
                return uniformBuffer == other.uniformBuffer;
            case BindingType::Sampler:
                return sampler == other.sampler;
            case BindingType::Texture:
                return texture == other.texture;
            case BindingType::SamplerTexture:
                break;
        }

        return false;
    }

};

struct RenderPipeline::Impl {
    VK::Device *device;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout descriptorSetLayout;
};

RenderPipeline::RenderPipeline() : impl(new Impl{}){}

RenderPipeline::~RenderPipeline() {
    deinit();
    delete impl;
}

static std::vector<VkVertexInputBindingDescription> getVertexInputBindingDescription(const RenderPipelineDescriptor &descriptor) {

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

static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescription(const RenderPipelineDescriptor &descriptor) {
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    for (uint32_t i = 0; i < descriptor.vertexDescriptor.attributes.count(); ++i) {
        VkVertexInputAttributeDescription description{};
        description.location = i;
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

static VkDescriptorType toVkDescriptorType(BindingType bindingType) {
    switch (bindingType) {
        case BindingType::Uniform:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case BindingType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case BindingType::Texture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case BindingType::SamplerTexture:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

bool RenderPipeline::init(const RenderPipelineDescriptor &renderPipelineDescriptor) {


    const RenderContext &renderContext = GetRenderer().getRenderContext();

    impl->device = renderContext.graphicContext->device;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = renderPipelineDescriptor.vertexFunction.library->impl->module;
    vertShaderStageInfo.pName = renderPipelineDescriptor.vertexFunction.name;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = renderPipelineDescriptor.fragmentFunction.library->impl->module;
    fragShaderStageInfo.pName = renderPipelineDescriptor.fragmentFunction.name;


    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    std::vector<VkVertexInputBindingDescription>  vertexInputBindingDescriptions = getVertexInputBindingDescription(renderPipelineDescriptor);
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions = getVertexInputAttributeDescription(renderPipelineDescriptor);

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
    rasterizer.cullMode = toVkCullMode(renderPipelineDescriptor.cullMode);
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
    multisampling.minSampleShading = 0.4f; // min fraction for sample shading; closer to one is smooth
    multisampling.rasterizationSamples = (VkSampleCountFlagBits)renderPipelineDescriptor.rasterSampleCount;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = renderPipelineDescriptor.alphaToCoverageEnabled;
    multisampling.alphaToOneEnable = renderPipelineDescriptor.alphaToOneEnabled;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderPipelineDescriptor.colorAttachments.count());
    for (uint32_t i = 0; i < colorBlendAttachments.size(); ++i) {
        colorBlendAttachments[i].colorWriteMask = toVkColorComponentFlags(renderPipelineDescriptor.colorAttachments[i].writeMask);

        colorBlendAttachments[i].blendEnable = renderPipelineDescriptor.colorAttachments[i].blendingEnabled;

        colorBlendAttachments[i].srcColorBlendFactor = toVkBlendFactor(renderPipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor);
        colorBlendAttachments[i].dstColorBlendFactor = toVkBlendFactor(renderPipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor);
        colorBlendAttachments[i].colorBlendOp = toVkBlendOp(renderPipelineDescriptor.colorAttachments[i].rgbBlendOperation);

        colorBlendAttachments[i].srcAlphaBlendFactor = toVkBlendFactor(renderPipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor);
        colorBlendAttachments[i].dstAlphaBlendFactor = toVkBlendFactor(renderPipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor);
        colorBlendAttachments[i].alphaBlendOp = toVkBlendOp(renderPipelineDescriptor.colorAttachments[i].alphaBlendOperation);

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
    depthStencil.depthTestEnable = renderPipelineDescriptor.depthStencilDescriptor.depthTestEnabled;
    depthStencil.depthWriteEnable = renderPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled;
    depthStencil.depthCompareOp = toVkCompareOp(renderPipelineDescriptor.depthStencilDescriptor.depthCompareFunction);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional


    VK::DescriptorSetLayoutDescriptor descriptorSetLayoutDescriptor;
    for (uint32_t i = 0; i < renderPipelineDescriptor.bindings.count(); ++i) {
        VK::DescriptorSetDescriptor descriptorSetDescriptor;
        descriptorSetDescriptor.type = toVkDescriptorType(renderPipelineDescriptor.bindings[i]);
        descriptorSetDescriptor.binding = i;
        descriptorSetDescriptor.arraySize = 1;
        descriptorSetDescriptor.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptorSetLayoutDescriptor.descriptorSetDescriptors.push_back(descriptorSetDescriptor);
    }

    impl->descriptorSetLayout = renderContext.graphicContext->device->getRenderResourceCache()
                                        .newDescriptorSetLayout(descriptorSetLayoutDescriptor)
                                        .vkDescriptorSetLayout();


    VkPushConstantRange push_constant;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = &impl->descriptorSetLayout;

    if (renderPipelineDescriptor.pushConstantEnabled) {
        push_constant.size = renderPipelineDescriptor.pushConstantDescriptor.size;
        push_constant.offset = renderPipelineDescriptor.pushConstantDescriptor.offset;
        push_constant.stageFlags = toVkPipelineStageFlags(renderPipelineDescriptor.pushConstantDescriptor.stageFlag);

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &push_constant;
    }

    if (vkCreatePipelineLayout(renderContext.graphicContext->device->vkDevice(), &pipelineLayoutInfo, nullptr, &impl->pipelineLayout) != VK_SUCCESS) {
        ANLog("failed to create pipeline layout!");
        return false;
    }


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
    pipelineInfo.layout = impl->pipelineLayout;
    pipelineInfo.renderPass = renderContext.graphicContext->renderCommandEncoder->getRenderPass().vkRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(renderContext.graphicContext->device->vkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &impl->graphicsPipeline) != VK_SUCCESS) {
        ANLog("failed to create graphics pipeline!");
        return false;
    }



    return true;
}


void RenderPipeline::deinit() {
    if (impl) {
        if (impl->graphicsPipeline) {
            vkDestroyPipeline(impl->device->vkDevice(), impl->graphicsPipeline, nullptr);
            impl->graphicsPipeline = nullptr;
        }

        if (impl->pipelineLayout) {
            vkDestroyPipelineLayout(impl->device->vkDevice(), impl->pipelineLayout, nullptr);
            impl->pipelineLayout = nullptr;
        }
    }
}



//static uint32_t lastFrameCount = -1;

//void RenderPipeline::bind() {
//    const RenderContext &context = GetRenderer().getRenderContext();
//    if (CurrentPipeline != this || lastFrameCount != context.frameCount) {
//        vkCmdBindPipeline(context.graphicContext->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, impl->graphicsPipeline);
//        VkViewport viewport{};
//        viewport.minDepth = 0.0;
//        viewport.maxDepth = 1.0;
//        viewport.width    = (float) context.frameWidth;
//        viewport.height   = (float) context.frameHeight;
//        vkCmdSetViewport(context.graphicContext->commandBuffer, 0, 1, &viewport);
//
//        VkRect2D scissor{};
//        scissor.offset = {0, 0};
//        scissor.extent = {.width = (uint32_t) context.frameWidth, .height = (uint32_t) context.frameHeight };
//
//        vkCmdSetScissor(context.graphicContext->commandBuffer, 0, 1, &scissor);
//
//        lastFrameCount = context.frameCount;
//
//        if (CurrentPipeline != this) {
//            GetRenderer().didChangeRenderPipeline(*this);
//        }
//
//        CurrentPipeline = this;
//    }
//}

void *RenderPipeline::getVkDescriptorLayout() {
    return impl->descriptorSetLayout;
}

void *RenderPipeline::getVkPipelineLayout() {
    return impl->pipelineLayout;
}

void *RenderPipeline::getVkPipeline() {
    return impl->graphicsPipeline;
}

}