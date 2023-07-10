//
// Created by aojoie on 4/17/2023.
//

#include "Render/ShaderFunction.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/ShaderFunction.hpp"
#endif

namespace AN::RC {


bool ShaderFunction::init(const UInt8 *code, size_t size) {
#ifdef OJOIE_USE_VULKAN
    impl = new VK::ShaderFunction();
#endif
    return impl && impl->init(code, size);
}

void ShaderFunction::deinit() {
    if (impl) {
        impl->deinit();
        delete impl;
        impl = nullptr;
    }
}


}