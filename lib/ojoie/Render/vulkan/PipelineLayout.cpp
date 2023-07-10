//
// Created by aojoie on 9/20/2022.
//

#include "Render/private/vulkan/PipelineLayout.hpp"
#include "Render/private/vulkan/DescriptorSetLayout.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

bool PipelineLayout::initCommon(const PipelineReflection &reflection, bool dynamicResources) {
    return initCommon(reflection.getResources(), dynamicResources);
}

bool PipelineLayout::initCommon(std::span<const ShaderResource> resources, bool dynamic) {
    // Create a descriptor set layout for each shader set in the shader program
    UInt32 maxSet = std::max_element(resources.begin(), resources.end(), [](auto &&a, auto &&b) {
                        return a.set < b.set;
                    })->set;

    for (UInt32 set = 0; set <= maxSet; ++set) {
        auto setResources = resources | std::views::filter([set](auto &&res) {
                                return res.resourceType != kShaderResourcePushConstant &&
                                       res.resourceType != kShaderResourceSpecializationConstant &&
                                       res.set == set;
                                ;
                            });
        if (setResources.empty()) {
            descriptor_set_layouts.emplace(0, &GetRenderResourceCache()
                                                       .newDescriptorSetLayout(setResources, dynamic)); // empty set layout
            continue;
        }
        descriptor_set_layouts.emplace(set,
                                       &GetRenderResourceCache()
                                                .newDescriptorSetLayout(setResources, dynamic));
    }

    // Collect all the descriptor set layout handles
    auto handles = descriptor_set_layouts | std::views::transform([](auto &&descriptor_set_layout_it) {
                       return descriptor_set_layout_it.second->vkDescriptorSetLayout();
                   });

    std::vector<VkDescriptorSetLayout> descriptor_set_layout_handles(handles.begin(), handles.end());

    // Collect all the push constant shader resources
    auto push_constant_resources = resources | std::views::filter([](auto &&res) {
                                       return res.resourceType == kShaderResourcePushConstant;
                                   });
    for (const auto &push_constant_resource : push_constant_resources) {
        pushConstantRanges.push_back({push_constant_resource.stages,
                                      (UInt32) push_constant_resource.block.offset,
                                      (UInt32) push_constant_resource.block.size});
    }

    VkPipelineLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    create_info.setLayoutCount         = (uint32_t) (descriptor_set_layout_handles.size());
    create_info.pSetLayouts            = descriptor_set_layout_handles.data();
    create_info.pushConstantRangeCount = (uint32_t) (pushConstantRanges.size());
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


}// namespace AN::VK