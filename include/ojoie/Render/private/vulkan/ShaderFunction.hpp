//
// Created by aojoie on 4/17/2023.
//

#ifndef OJOIE_VK_SHADERFUNCTION_HPP
#define OJOIE_VK_SHADERFUNCTION_HPP

#include <ojoie/Render/ShaderFunction.hpp>
#include <ojoie/Render/private/vulkan.hpp>

namespace AN::VK {

class ShaderFunction : public RC::ShaderFunctionImpl, private NonCopyable {
    VkShaderModule shaderModule{};
public:

    ~ShaderFunction() {
        deinit();
    }

    virtual bool init(const UInt8 *code, size_t size) override;
    virtual void deinit() override;

    VkShaderModule getVkShaderModule() const { return shaderModule; }
};


}

#endif//OJOIE_VK_SHADERFUNCTION_HPP
