//
// Created by aojoie on 4/15/2023.
//

#ifndef OJOIE_SHADER_HPP
#define OJOIE_SHADER_HPP

#include <ojoie/Asset/TextAsset.hpp>
#include <ojoie/Render/RenderPipelineState.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/ShaderFunction.hpp>
#include <ojoie/ShaderLab/Parser.hpp>
#include <ojoie/Template/RC.hpp>
#include <ojoie/Template/SmallVector.hpp>


#include <any>
#include <span>

namespace AN {

/// commandBuffer will use _renderPipelineState and _renderPassDescriptor to
/// create real RenderPass and real FrameBuffers
/// if the shader has more than one pass, this could be an array

/// Shader object can be created in any thread
class AN_API Shader : public TextAsset {

public:
    /// currently not support pushConstant block
    enum BindingType {
        kBindingTypeTexture = 0,
        kBindingTypeSampler,
        kBindingTypeBuffer
    };

    struct BindingInfo {
        UInt32      set;
        UInt32      binding;
        BindingType bindingType;
        Name        name;
        UInt32      size;// if type is buffer
        ShaderStage stage;

        inline static const char *GetTypeString() { return "BindingInfo"; }

        template<typename Coder>
        void transfer(Coder &coder) {
            TRANSFER(set);
            TRANSFER(binding);
            coder.transfer((std::underlying_type_t<BindingType> &) bindingType, "bindingType");
            TRANSFER(name);
            TRANSFER(size);
        }
    };

    struct Pass {

        RenderPipelineState           *renderPipelineState = nullptr;
        ShaderPropertyList              propertyList;
        SmallVector<ShaderVertexInput> vertexInputs;
        std::vector<BindingInfo>       bindingInfos;


        /// serialize data below
        ShaderLab::ShaderPass shaderLabPass;
        /// the compiled spv code
        std::vector<UInt8> vertex_spv, fragment_spv;


        template<typename Coder>
        void transfer(Coder &coder) {
            TRANSFER(shaderLabPass);
            size_t size = vertex_spv.size();
            coder.transferTypeless(size, "vertex_spv_size");
            if constexpr (Coder::IsDecoding()) {
                vertex_spv.resize(size);
            }
            coder.transferTypelessData(vertex_spv.data(), size);
            size = fragment_spv.size();
            coder.transferTypeless(size, "fragment_spv_size");
            if constexpr (Coder::IsDecoding()) {
                fragment_spv.resize(size);
            }
            coder.transferTypelessData(fragment_spv.data(), size);
        }
    };

    struct SubShader {
        ShaderLab::TagMap shaderLabTagMap;
        SmallVector<Pass> passes;

        template<typename Coder>
        void transfer(Coder &coder) {
            TRANSFER(shaderLabTagMap);
            TRANSFER(passes);
        }
    };

private:
    std::string              _scriptPath;
    SmallVector<std::string> _includes;

    SmallVector<SubShader> subShaders;

    std::vector<ShaderLab::Property> shaderLabProperties;

    DECLARE_DERIVED_AN_CLASS(Shader, TextAsset)
    DECLARE_OBJECT_SERIALIZE(Shader)

public:
    explicit Shader(ObjectCreationMode mode);

    virtual void dealloc() override;

    bool initWithScript(std::string_view scriptPath, std::span<const char *> includes = {});

    bool initWithScriptText(const char *text, std::span<const char *> includes = {});

    virtual bool initAfterDecode() override;

    /// set script force recompile
    bool setScript(std::string_view scriptPath, std::span<const char *> includes = {});

    bool setScriptText(const char *text, std::span<const char *> includes = {});

    virtual std::string getTextAssetPath() override;
    virtual void        setTextAssetPath(std::string_view path) override;

    /// after init or setScript must call this method to use in material
    bool createGPUObject();

    void destroyGPUObject();

    UInt32 getSubShaderNum() const { return subShaders.size(); }
    UInt32 getPassNum(UInt32 subShader) const { return subShaders[subShader].passes.size(); }

    RenderPipelineState &getPassRenderPipelineState(UInt32 passIndex, UInt32 subShaderIndex);

    std::span<const ShaderVertexInput> getVertexInputs(UInt32 passIndex, UInt32 subShaderIndex) const;

    /// retrieve a view of properties
    const ShaderPropertyList &getProperties(UInt32 passIndex, UInt32 subShaderIndex) const {
        return subShaders[subShaderIndex].passes[passIndex].propertyList;
    }

    std::span<const BindingInfo> getBindingInfo(UInt32 passIndex, UInt32 subShaderIndex) const {
        return subShaders[subShaderIndex].passes[passIndex].bindingInfos;
    }

    const BindingInfo *getUniformBufferInfo(UInt32 passIndex, UInt32 subShaderIndex, ShaderStage stage, UInt32 binding, UInt32 set = 0) {
        auto &infos = subShaders[subShaderIndex].passes[passIndex].bindingInfos;
        auto  it    = std::find_if(infos.begin(),
                                   infos.end(), [binding, set, stage](const BindingInfo &info) {
                                   return info.set == set &&
                                          info.binding == binding &&
                                          info.stage == stage &&
                                          info.bindingType == kBindingTypeBuffer;
                               });
        if (it != infos.end()) {
            return &*it;
        }

        return nullptr;
    }

    const std::vector<ShaderLab::Property> &getShaderLabProperties() const {
        return shaderLabProperties;
    }

    /// get pass index according to LightMode name, return -1 if not found
    UInt32 getPassIndex(const char *name, UInt32 subShaderIndex = 0) {
        const auto &passes = subShaders[subShaderIndex].passes;
        for (int i = 0; i < passes.size(); ++i) {
            if (passes[i].shaderLabPass.tagMap.at("LightMode").string_view() == name) {
                return i;
            }
        }
        return -1;
    }

#ifdef OJOIE_WITH_EDITOR
    void onInspectorGUI();
#endif
};


}// namespace AN

#endif//OJOIE_SHADER_HPP
