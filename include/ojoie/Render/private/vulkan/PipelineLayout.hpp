//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_PIPELINELAYOUT_HPP
#define OJOIE_VK_PIPELINELAYOUT_HPP

#include "ojoie/Render/private/vulkan.hpp"
#include <ojoie/Render/PipelineReflection.hpp>

namespace AN::VK {

class DescriptorSetLayout;

class PipelineLayout : private NonCopyable {
    Device *_device;
    VkPipelineLayout pipelineLayout{};
//    ShaderProgram shaderProgram;
    std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layouts;

    std::vector<VkPushConstantRange> pushConstantRanges;

    bool initCommon(const PipelineReflection &reflection, bool dynamicResources);
    bool initCommon(std::span<const ShaderResource> resources, bool dynamic);
public:

    PipelineLayout() = default;

    PipelineLayout(PipelineLayout &&other) noexcept
        : _device(other._device), pipelineLayout(other.pipelineLayout),
          descriptor_set_layouts(std::move(other.descriptor_set_layouts)),
          pushConstantRanges(std::move(other.pushConstantRanges)){
        other.pipelineLayout = nullptr;
    }

    ~PipelineLayout() {
        deinit();
    }

    bool init(Device &device, const PipelineReflection &reflection, bool dynamicResources) {
        _device = &device;
        return initCommon(reflection, dynamicResources);
    }

    bool init(Device &device, std::span<const ShaderResource> resources, bool dynamic) {
        _device = &device;
        return initCommon(resources, dynamic);
    }

    void deinit();

    VkPipelineLayout vkPipelineLayout() const {
        return pipelineLayout;
    }

//    const ShaderProgram &getShaderProgram() const {
//        return shaderProgram;
//    }

    VkShaderStageFlags getPushConstantRangeStage(uint32_t offset, uint32_t size) const {
        VkShaderStageFlags stages = 0;

        for (auto &push_constant_resource : pushConstantRanges) {
            if (offset >= push_constant_resource.offset &&
                offset + size <= push_constant_resource.offset + push_constant_resource.size) {
                stages |= push_constant_resource.stageFlags;
            }
        }
        return stages;
    }

    bool hasDescriptorSetLayout(uint32_t set_index) const {
        return descriptor_set_layouts.contains(set_index);
    }

    DescriptorSetLayout &getDescriptorSetLayout(uint32_t set_index) const {
        return *descriptor_set_layouts.at(set_index);
    }

    const std::unordered_map<uint32_t, DescriptorSetLayout *> &getDescriptorSetLayouts() const {
        return descriptor_set_layouts;
    }
};

}

#endif//OJOIE_VK_PIPELINELAYOUT_HPP
