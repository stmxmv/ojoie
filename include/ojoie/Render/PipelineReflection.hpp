//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_PIPELINEREFLECTION_HPP
#define OJOIE_VK_PIPELINEREFLECTION_HPP

#include <ojoie/Core/Name.hpp>
#include <ojoie/Render/RenderTypes.hpp>

#include <algorithm>
#include <ranges>
#include <span>
#include <unordered_map>

namespace AN {

class AN_API PipelineReflection {
    std::vector<ShaderVertexInput> _vertexInputs;
    std::vector<ShaderResource>    _resources;

public:
    /// reflect a single SPIRV code which constains only one entry point
    /// can be vertex or fragment
    /// usually call this method twice to reflect both vertex and fragment shader
    bool reflectSPIRV(const void *spirv, uint64_t size);

    bool reflectCSO(const void *cso, uint64_t size, ShaderStage stage);

    /// we ensure that resources is sorted by set and then by binding, and binding number is continual
    /// note that some resources does not have set and binding, which will be zero
    std::span<const ShaderResource> getResources() const { return _resources; }

    std::span<const ShaderVertexInput> getVertexInputs() const { return _vertexInputs; }

    /// nullable
    const ShaderVertexInput *getVertexInput(const char *semantic) const;

    /// nullable
    const ShaderResource *getResource(const char *name, ShaderStage stage) const;

    auto getResourcesWithType(ShaderResourceType type) const {
        return _resources | std::views::filter([type](auto &&res) {
                   return res.resourceType == type;
               });
    }

    /// filter resources that will in a set and binding
    auto getResourcesSets() const {
        return _resources | std::views::filter([](auto &&res) {
                   return res.resourceType != kShaderResourcePushConstant &&
                          res.resourceType != kShaderResourceSpecializationConstant;
               });
    }

    /// pushConstant and SpecializationConstant will be not in the result
    auto getResourcesWithSet(UInt32 set) const {
        return _resources | std::views::filter([set](auto &&res) {
                   return res.resourceType != kShaderResourcePushConstant &&
                          res.resourceType != kShaderResourceSpecializationConstant &&
                          res.set == set;
               });
    }

    UInt32 getMaxDescriptorSet() const {
        auto it = std::max_element(_resources.begin(), _resources.end(), [](auto &&a, auto &&b) {
            return a.set < b.set;
        });
        return it->set;
    }


    ShaderPropertyList buildPropertyList() const;
};


}// namespace AN

#endif//OJOIE_VK_PIPELINEREFLECTION_HPP
