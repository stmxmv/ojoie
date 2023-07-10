//
// Created by aojoie on 5/22/2023.
//

#include "Render/Layer.hpp"
#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/Layer.hpp"
#endif //OJOIE_USE_VULKAN

#include "Render/private/D3D11/Layer.hpp"

namespace AN {

Layer *Layer::Alloc() {
    GraphicsAPI api = GetGraphicsAPI();
#ifdef _WIN32
    if (api == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        return new VK::Win32Layer();
#endif //OJOIE_USE_VULKAN
    }
    if (api == kGraphicsAPID3D11) {
        return new D3D11::Layer();
    }


#else
#error "not implement"
#endif

    return nullptr;
}

}