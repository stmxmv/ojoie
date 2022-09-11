//
// Created by Aleudillonam on 9/9/2022.
//

#ifndef OJOIE_VK_RENDERPIPELINE_HPP
#define OJOIE_VK_RENDERPIPELINE_HPP

#include "Render/private/vulkan.hpp"

namespace AN::VK {

class Device;

class RenderPipeline : private NonCopyable {

    Device *_device;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline{};
    VkDescriptorSetLayout descriptorSetLayout;

public:

    RenderPipeline() = default;


};

}

#endif//OJOIE_VK_RENDERPIPELINE_HPP
