//
// Created by aojoie on 4/15/2023.
//

#include "Render/Shader/Shader.hpp"

#include "Render/VertexData.hpp"
#include "ShaderLab/Parser.hpp"


#include <ojoie/Utility/Log.h>
#include <ojoie/Render/Shader/ShaderCompiler.hpp>
#include <ojoie/Utility/SourceFile.hpp>
#include <ojoie/Render/QualitySettings.hpp>
#include <ojoie/Render/RenderContext.hpp>

#include <ojoie/Threads/Dispatch.hpp>
#include "HAL/File.hpp"
#include "Render/private/D3D11/UniformBuffers.hpp"


#ifdef OJOIE_WITH_EDITOR
#include <ojoie/IMGUI/IMGUI.hpp>
#endif

#ifdef OJOIE_USE_SPIRV
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#endif//OJOIE_USE_SPIRV
#include <filesystem>

namespace AN {

static void initRenderPassDescriptor(RenderPassDescriptor &renderPassDescriptor,
                                     AntiAliasingMethod antiAliasingMethod,
                                     UInt32 msaaSamples) {


//    if (antiAliasingMethod == kAntiAliasingMSAA) {
//
//        SubpassInfo subpass_infos[1] = {};
//        LoadStoreInfo load_store[3] = {};
//        ClearValue clearValue[3] = {};
//
//        // resolve swapchain image
//        load_store[0].loadOp = kAttachmentLoadOpDontCare; /// 0 swapchain image will be resolved anyway
//        load_store[0].storeOp = kAttachmentStoreOpStore;
//
//        // Depth
//        load_store[1].loadOp  = kAttachmentLoadOpClear;
//        load_store[1].storeOp = kAttachmentStoreOpDontCare;
//
//        // msaa image
//        load_store[2].loadOp  = kAttachmentLoadOpClear;
//        load_store[2].storeOp = kAttachmentStoreOpStore;
//
//        subpass_infos[0].colorAttachments = { 2 };
//        subpass_infos[0].resolveAttachment = 0;
//        subpass_infos[0].depthStencilAttachment = 1;
//
//        /// target image's clear value is ignored anyway because we set it DontCare above
//        clearValue[1].depthStencil = { 1.0f, 0 };
//        clearValue[2].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//
//
//        RenderAttachment attachment[2];
//        attachment[0].samples = msaaSamples;
//        attachment[0].format = kRTFormatDepth;
//
//        attachment[1].samples = msaaSamples;
//        attachment[1].format = kRTFormatDefault; /// default will use render target's format
//
//        renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
//        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));
//        renderPassDescriptor.clearValues.assign(std::begin(clearValue), std::end(clearValue));
//
//        renderPassDescriptor.attachments.emplace_back(); // 0 for render target, which is unknown for now
//        renderPassDescriptor.attachments.insert(renderPassDescriptor.attachments.end(),
//                                                 std::begin(attachment), std::end(attachment));
//
//    } else {
//
//        /// init render pass descriptor
//        SubpassInfo subpass_infos[1] = {};
//        LoadStoreInfo load_store[2] = {};
//
//        // Swapchain
//        load_store[0].loadOp  = kAttachmentLoadOpClear;
//        load_store[0].storeOp = kAttachmentStoreOpStore;
//
//        // Depth
//        load_store[1].loadOp  = kAttachmentLoadOpClear;
//        load_store[1].storeOp = kAttachmentStoreOpDontCare;
//
//        subpass_infos[0].colorAttachments = { 0 };
//        subpass_infos[0].depthStencilAttachment = 1;
//
//
//        ClearValue clearValue[2];
//        clearValue[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//        clearValue[1].depthStencil = { 1.0f, 0 };
//
//        renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
//        renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));
//        renderPassDescriptor.clearValues.assign(std::begin(clearValue), std::end(clearValue));
//
//        RenderAttachment depthAttachment{};
//        depthAttachment.samples = msaaSamples;
//        depthAttachment.format = kRTFormatDepth;
//
//        renderPassDescriptor.attachments.emplace_back(); // 0 for render target, which is unknown for now
//        renderPassDescriptor.attachments.push_back(depthAttachment);
//
//    }

}

IMPLEMENT_AN_CLASS(Shader)
LOAD_AN_CLASS(Shader)


template<typename _Coder>
void Shader::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(_scriptPath);
    TRANSFER(_includes);
    TRANSFER(subShaders);
    TRANSFER(shaderLabProperties);
}

IMPLEMENT_OBJECT_SERIALIZE(Shader)
INSTANTIATE_TEMPLATE_TRANSFER(Shader)


Shader::Shader(ObjectCreationMode mode) : Super(mode) {}
Shader::~Shader() {}

void Shader::dealloc() {
    destroyGPUObject();
    subShaders.clear();
    Super::dealloc();
}


void Shader::destroyGPUObject() {
    /// may pending destroy the subShaders
    for (int i = 0; i < subShaders.size(); ++i) {
        for (int j = 0; j < subShaders[i].passes.size(); ++j) {
            auto &pass = subShaders[i].passes[j];
            if (pass.renderPipelineState != nullptr) {
                pass.renderPipelineState->deinit();
                delete pass.renderPipelineState;
                pass.renderPipelineState = nullptr;
            }
        }
    }
}

bool Shader::setScript(std::string_view scriptPath, std::span<const char *> includes) {
    SourceFile sourceFile;
    if (!sourceFile.init(scriptPath)) {
        return false;
    }

    std::filesystem::path path(scriptPath);
    auto                  fileName = path.filename().string();

    _scriptPath = scriptPath;

    return setScriptText(sourceFile.getBuffer(), includes);
}

bool Shader::setScriptText(const char *text, std::span<const char *> includes) {
    _includes.clear();

    std::vector<const char *> includePaths;
    for (const char *inc : includes) {
        _includes.emplace_back(inc);
        includePaths.push_back(inc);
    }

#undef GetCurrentDirectory

    /// adding default include path
    std::string curDir = GetApplicationFolder();
    std::string stdlibDir = curDir + "/Data/CGIncludes/stdlib";
    includePaths.push_back(stdlibDir.c_str());


    ShaderLab::Lexer  lexer(text);
    ShaderLab::Parser parser(lexer);

    ShaderLab::ShaderInfo shaderInfo = parser.parse();

    if (parser.hasError()) {
        AN_LOG(Error, "%s", parser.getError().getDescription().c_str());
        return false;
    }

    setName(shaderInfo.name.string_view());

    shaderLabProperties = shaderInfo.properties;

    subShaders.clear();

    int subShaderIndex = 0;
    for (const ShaderLab::SubShader &shaderLabSubShader : shaderInfo.subShaders) {
        subShaders.emplace_back();
        SubShader &subShader = subShaders.back();
        subShader.shaderLabTagMap = shaderLabSubShader.tagMap;
        int passIndex = 0;
        for (const ShaderLab::ShaderPass &shaderLabPass : shaderLabSubShader.passes) {
            subShader.passes.emplace_back();
            Pass &pass = subShader.passes.back();
            pass.shaderLabPass = shaderLabPass;

            std::string realSource;
            realSource.reserve(shaderInfo.subShaderHLSLIncludes[subShaderIndex].size() +
                               shaderInfo.passHLSLSources[subShaderIndex][passIndex].size());
            realSource.append(shaderInfo.subShaderHLSLIncludes[subShaderIndex]);
            realSource.append("\r\n");
            realSource.append(shaderInfo.passHLSLSources[subShaderIndex][passIndex]);

            RC::ShaderCompiler shaderCompiler;

            if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
                pass.vertex_spv = shaderCompiler.compileHLSLToSPIRV(kShaderStageVertex,
                                                                    realSource.c_str(),
                                                                    "vertex_main",
                                                                    shaderInfo.name.c_str(),
                                                                    includePaths);

                if (pass.vertex_spv.empty()) return false;

                pass.fragment_spv = shaderCompiler.compileHLSLToSPIRV(kShaderStageFragment,
                                                                      realSource.c_str(),
                                                                      "fragment_main",
                                                                      shaderInfo.name.c_str(),
                                                                      includePaths);
                if (pass.fragment_spv.empty()) return false;

            } else {
                pass.vertex_spv = shaderCompiler.compileHLSLToCSO(kShaderStageVertex,
                                                                  realSource.c_str(),
                                                                  "vertex_main",
                                                                  shaderInfo.name.c_str(),
                                                                  includePaths);

                if (pass.vertex_spv.empty()) return false;

                pass.fragment_spv = shaderCompiler.compileHLSLToCSO(kShaderStageFragment,
                                                                    realSource.c_str(),
                                                                    "fragment_main",
                                                                    shaderInfo.name.c_str(),
                                                                    includePaths);
                if (pass.fragment_spv.empty()) return false;

            }
            ++passIndex;
        }
        ++subShaderIndex;
    }
    return true;
}


bool Shader::createGPUObject() {
    for (int i = 0; i < subShaders.size(); ++i) {
        for (int j = 0; j < subShaders[i].passes.size(); ++j) {
            auto &pass = subShaders[i].passes[j];
            PipelineReflection pipelineReflection;

            if (GetGraphicsAPI() == kGraphicsAPIVulkan) {

                if (!pipelineReflection.reflectSPIRV(pass.vertex_spv.data(), pass.vertex_spv.size())) return false;
                if (!pipelineReflection.reflectSPIRV(pass.fragment_spv.data(), pass.fragment_spv.size())) return false;

#ifdef OJOIE_USE_SPIRV
                /// remove reflect info and debug info
                /// remove reflect info is needed, due to some vulkan layer not support it
                spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_3);
                auto print_msg_to_stderr = [](spv_message_level_t, const char*,
                                              const spv_position_t&, const char* m) {
                    AN_LOG(Error, "%s", m);
                };
                opt.SetMessageConsumer(print_msg_to_stderr);

                opt.RegisterPass(spvtools::CreateStripNonSemanticInfoPass())
                        .RegisterPass(spvtools::CreateStripDebugInfoPass());

                std::vector<uint32_t> optimized_binary;
                if (!opt.Run((const uint32_t *)pass.vertex_spv.data(),
                             pass.vertex_spv.size() / sizeof(uint32_t),
                             &optimized_binary))
                    return false;

                pass.vertex_spv.resize(optimized_binary.size() * sizeof(uint32_t));
                memcpy(pass.vertex_spv.data(), optimized_binary.data(), pass.vertex_spv.size());

                optimized_binary.clear();

                /// optimizer cannot reuse, because I've tested it
                spvtools::Optimizer fragOpt(SPV_ENV_UNIVERSAL_1_3);
                fragOpt.RegisterPass(spvtools::CreateStripNonSemanticInfoPass())
                        .RegisterPass(spvtools::CreateStripDebugInfoPass());
                if (!fragOpt.Run((const uint32_t *)pass.fragment_spv.data(),
                                 pass.fragment_spv.size() / sizeof(uint32_t),
                                 &optimized_binary))
                    return false;


                pass.fragment_spv.resize(optimized_binary.size() * sizeof(uint32_t));
                memcpy(pass.fragment_spv.data(), optimized_binary.data(), pass.fragment_spv.size());

#endif//OJOIE_USE_SPIRV

            } else {

                if (!pipelineReflection.reflectCSO(pass.vertex_spv.data(), pass.vertex_spv.size(), kShaderStageVertex))
                    return false;
                if (!pipelineReflection.reflectCSO(pass.fragment_spv.data(), pass.fragment_spv.size(), kShaderStageFragment))
                    return false;
            }


            pass.vertexInputs.assign(pipelineReflection.getVertexInputs().begin(), pipelineReflection.getVertexInputs().end());
            pass.propertyList = pipelineReflection.buildPropertyList();

            /// note that pipelineReflection ensure that resources is sorted by set and binding
            for (const auto &resource : pipelineReflection.getResourcesSets()) {
                BindingInfo bindingInfo{};
                bindingInfo.set = resource.set;
                bindingInfo.binding = resource.binding;
                bindingInfo.name = resource.name;
                bindingInfo.stage = resource.stage;

                if (resource.resourceType == kShaderResourceBufferUniform ||
                    resource.resourceType == kShaderResourceBufferStorage) {

                    bindingInfo.bindingType = kBindingTypeBuffer;
                    bindingInfo.size = resource.block.size;

                    if (GetGraphicsAPI() == kGraphicsAPID3D11) {
                        D3D11::GetUniformBuffers().setBufferInfo(bindingInfo.name.getIndex(), bindingInfo.size);
                    }

                } else if (resource.resourceType == kShaderResourceImage ||
                           resource.resourceType == kShaderResourceImageStorage ||
                           resource.resourceType == kShaderResourceInputAttachment) {
                    bindingInfo.bindingType = kBindingTypeTexture;
                } else if (resource.resourceType == kShaderResourceSampler) {
                    bindingInfo.bindingType = kBindingTypeSampler;
                } else {
                    /// not support type
                    AN_LOG(Error, "not support shader resource type %d", bindingInfo.bindingType);
                }
                pass.bindingInfos.push_back(bindingInfo);
            }

            RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
            renderPipelineStateDescriptor.vertexFunction.entry = "vertex_main";
            renderPipelineStateDescriptor.vertexFunction.code  = pass.vertex_spv.data();
            renderPipelineStateDescriptor.vertexFunction.size  = pass.vertex_spv.size();

            renderPipelineStateDescriptor.fragmentFunction.entry = "fragment_main";
            renderPipelineStateDescriptor.fragmentFunction.code  = pass.fragment_spv.data();
            renderPipelineStateDescriptor.fragmentFunction.size  = pass.fragment_spv.size();

            RenderPipelineColorAttachmentDescriptor colorAttachmentDescriptor{};
            colorAttachmentDescriptor.blendingEnabled             = pass.shaderLabPass.blending;
            colorAttachmentDescriptor.rgbBlendOperation           = pass.shaderLabPass.blendOperation;
            colorAttachmentDescriptor.sourceRGBBlendFactor        = pass.shaderLabPass.sourceBlendFactor;
            colorAttachmentDescriptor.destinationRGBBlendFactor   = pass.shaderLabPass.destinationBlendFactor;
            colorAttachmentDescriptor.writeMask                   = pass.shaderLabPass.colorWriteMask;
            colorAttachmentDescriptor.sourceAlphaBlendFactor      = pass.shaderLabPass.sourceAlphaFactor;
            colorAttachmentDescriptor.destinationAlphaBlendFactor = pass.shaderLabPass.destinationAlphaFactor;
            colorAttachmentDescriptor.alphaBlendOperation         = kBlendOperationAdd;

            renderPipelineStateDescriptor.colorAttachments.push_back(colorAttachmentDescriptor);

            renderPipelineStateDescriptor.depthStencilDescriptor.depthTestEnabled = true;
            renderPipelineStateDescriptor.depthStencilDescriptor.depthWriteEnabled = pass.shaderLabPass.ZWrite;
            renderPipelineStateDescriptor.depthStencilDescriptor.depthCompareFunction = pass.shaderLabPass.ZTestOperation;
            /// TODO stencil

            /// anti-aliasing
            UInt32 samples = GetQualitySettings().getCurrent().antiAliasing;
            if (samples > 1) {
                renderPipelineStateDescriptor.rasterSampleCount = samples;
            } else {
                renderPipelineStateDescriptor.rasterSampleCount = 1;
            }


            renderPipelineStateDescriptor.alphaToOneEnabled = false;
            renderPipelineStateDescriptor.alphaToCoverageEnabled = false;

            renderPipelineStateDescriptor.cullMode = pass.shaderLabPass.cullMode;

            if (pass.renderPipelineState == nullptr) {
                pass.renderPipelineState = new RenderPipelineState();
            }

            if (!pass.renderPipelineState->init(renderPipelineStateDescriptor, pipelineReflection)) return false;

#ifndef OJOIE_WITH_EDITOR
            /// destroy shader code to save memory
            std::vector<UInt8>().swap(pass.vertex_spv);
            std::vector<UInt8>().swap(pass.fragment_spv);
#endif
        }
    }
    return true;
}

bool Shader::initWithScript(std::string_view scriptPath, std::span<const char *> includes) {
    if (!Super::init()) return false;
    return setScript(scriptPath, includes);
}

bool Shader::initWithScriptText(const char *text, std::span<const char *> includes) {
    if (!Super::init()) return false;
    return setScriptText(text, includes);
}

bool Shader::initAfterDecode() {
    if (!Super::initAfterDecode() || !createGPUObject()) return false;
    return true;
}

RenderPipelineState &Shader::getPassRenderPipelineState(UInt32 passIndex, UInt32 subShaderIndex) {
    return *subShaders[subShaderIndex].passes[passIndex].renderPipelineState;
}

std::span<const ShaderVertexInput> Shader::getVertexInputs(UInt32 passIndex, UInt32 subShaderIndex) const {
    return subShaders[subShaderIndex].passes[passIndex].vertexInputs;
}

std::string Shader::getTextAssetPath() {
    return _scriptPath;
}

void Shader::setTextAssetPath(std::string_view path) {
    _scriptPath = path;
}

#ifdef OJOIE_WITH_EDITOR
void Shader::onInspectorGUI() {
    const auto &properties = getShaderLabProperties();

    if (getName().string_view() == "Hidden/ErrorShader") {

        ImGui::Text("Shader compile error");


        return;
    }
    
    ImGui::Text("Shader Properties");
    for (const auto &prop : properties) {
        ImGui::PushID(&prop);
        ItemLabel(prop.name.c_str(), kItemLabelLeft);
        std::string propIDStr = std::format("##{}", prop.name.c_str());
        switch (prop.type) {
            case kShaderPropertyInt:
            {
                ImGui::Text("Int range(%d, %d) default value: %d", prop.range.int_min, prop.range.int_max, prop.defaultValue.intValue);
            }
            break;
            case kShaderPropertyFloat:
            {
                if (prop.dimension == 1) {
                    ImGui::Text("float range(%f, %f) default value: %f", prop.range.float_min, prop.range.float_max, prop.defaultValue.floatValue);
                } else if (prop.dimension == 4) {
                    ImGui::Text("Vector default value: (%f, %f, %f, %f)", prop.defaultValue.vector4f.x, prop.defaultValue.vector4f.y, prop.defaultValue.vector4f.z, prop.defaultValue.vector4f.w);
                } else {
                    AN_LOG(Error, "Error ShaderLab Property Float Dimension %d (Shader %p)", prop.dimension, this);
                }
            }
            break;
            case kShaderPropertyTexture:
            {
                ImGui::Text("Texture dimension %d default value: %s", prop.dimension, prop.defaultStringValue.c_str());
            }
            break;
            default:
                break;
        }
        ImGui::PopID();
    }
}
#endif

}// namespace AN