//
// Created by aojoie on 5/18/2023.
//

#include "Render/TextureManager.hpp"
#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/TextureManager.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/TextureManager.hpp"

namespace AN {


TextureManager &GetTextureManager() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        return VK::GetTextureManager();
#endif//OJOIE_USE_VULKAN
    } else {
        return D3D11::GetTextureManager();
    }
    return D3D11::GetTextureManager();
}

}