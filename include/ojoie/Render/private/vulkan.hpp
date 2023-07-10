//
// Created by Aleudillonam on 8/13/2022.
//

#ifndef OJOIE_VULKAN_HPP
#define OJOIE_VULKAN_HPP

#include <ojoie/Render/RenderContext.hpp>

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <unordered_map>

namespace AN {

namespace VK {

class RenderCommandEncoder;
class Device;

const char *ResultCString(VkResult result);

class Exception : public AN::Exception {
    typedef AN::Exception Super;

public:
#ifdef __cpp_lib_source_location
    Exception(VkResult result, const char *message, const std::source_location location = std::source_location::current()) noexcept : Super(std::format("Vulkan return error {}, message {}", ResultCString(result), message).c_str(), location) {}

#else
    explicit Exception(VkResult result, const char *message) noexcept : Super(std::format("Vulkan Error code {}, message {}", (int)result, message).c_str()) {}

#endif
};

}


struct GraphicContext {
    VK::Device *device;
    VkCommandBuffer commandBuffer; // current rendering commandBuffer
    VK::RenderCommandEncoder *renderCommandEncoder; // current commandBuffer associated render command encoder
};

enum CommandBufferResetMode {
    kCommandBufferResetModeResetPool,
    kCommandBufferResetModeResetIndividually
};


}

#endif//OJOIE_VULKAN_HPP
