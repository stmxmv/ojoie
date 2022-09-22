//
// Created by aojoie on 9/20/2022.
//

#include "Render/private/vulkan/PipelineLayout.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/DescriptorSetLayout.hpp"

namespace AN::VK {

bool PipelineLayout::initCommon(bool dynamicResources) {
    // Create a descriptor set layout for each shader set in the shader program
    for (const auto &shader_set_it : shaderProgram.getDescriptorSets()) {
        descriptor_set_layouts.emplace(shader_set_it.first,
                                       &_device->getRenderResourceCache()
                                                .newDescriptorSetLayout(shader_set_it.second, dynamicResources));
    }

    // Collect all the descriptor set layout handles
    std::vector<VkDescriptorSetLayout> descriptor_set_layout_handles(descriptor_set_layouts.size());
    std::transform(descriptor_set_layouts.begin(), descriptor_set_layouts.end(), descriptor_set_layout_handles.begin(),
                   [](auto &descriptor_set_layout_it) { return descriptor_set_layout_it.second->vkDescriptorSetLayout(); });

    // Collect all the push constant shader resources
    auto push_constant_resources = shaderProgram.getResources(ShaderResourceType::PushConstant);
    for (auto &push_constant_resource : push_constant_resources) {
        pushConstantRanges.push_back({push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size});
    }

    VkPipelineLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    create_info.setLayoutCount         = (uint32_t)(descriptor_set_layout_handles.size());
    create_info.pSetLayouts            = descriptor_set_layout_handles.data();
    create_info.pushConstantRangeCount = (uint32_t)(pushConstantRanges.size());
    create_info.pPushConstantRanges    = pushConstantRanges.data();

    // Create the Vulkan pipeline layout handle
    auto result = vkCreatePipelineLayout(_device->vkDevice(), &create_info, nullptr, &pipelineLayout);

    if (result != VK_SUCCESS) {
        ANLog("Cannot create PipelineLayout vulkan result %s", ResultCString(result));
        return false;
    }


    return true;
}

void PipelineLayout::deinit() {
    if (pipelineLayout) {
        vkDestroyPipelineLayout(_device->vkDevice(), pipelineLayout, nullptr);
        pipelineLayout = nullptr;
    }
}



}