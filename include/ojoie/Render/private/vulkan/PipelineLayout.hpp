//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_PIPELINELAYOUT_HPP
#define OJOIE_VK_PIPELINELAYOUT_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/ShaderProgram.hpp"

namespace AN::VK {

class DescriptorSetLayout;

class PipelineLayout : private NonCopyable {
    Device *_device;
    VkPipelineLayout pipelineLayout{};
    ShaderProgram shaderProgram;
    std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layouts;

    std::vector<VkPushConstantRange> pushConstantRanges;

    bool initCommon(bool dynamicResources);
public:

    PipelineLayout() = default;

    PipelineLayout(PipelineLayout &&other) noexcept
        : _device(other._device), pipelineLayout(other.pipelineLayout), shaderProgram(std::move(other.shaderProgram)),
          descriptor_set_layouts(std::move(other.descriptor_set_layouts)),
          pushConstantRanges(std::move(other.pushConstantRanges)){
        other.pipelineLayout = nullptr;
    }

    ~PipelineLayout() {
        deinit();
    }

    template<typename Functions>
    bool init(Device &device, Functions &&funcs, bool dynamicResources) {
        _device = &device;
        if (!shaderProgram.init(std::forward<Functions>(funcs))) {
            return false;
        }
        return initCommon(dynamicResources);
    }

    bool init(Device &device, ShaderFunction *funcs, uint64_t size, bool dynamicResources) {
        _device = &device;
        if (!shaderProgram.init(funcs, size)) {
            return false;
        }
        return initCommon(dynamicResources);
    }

    void deinit();

    VkPipelineLayout vkPipelineLayout() const {
        return pipelineLayout;
    }

    const ShaderProgram &getShaderProgram() const {
        return shaderProgram;
    }

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
