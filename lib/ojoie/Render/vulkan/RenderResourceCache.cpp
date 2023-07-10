//
// Created by aojoie on 4/17/2023.
//

#include "Render/private/vulkan/RenderResourceCache.hpp"

namespace AN::VK {


RenderResourceCache &GetRenderResourceCache() {
    static RenderResourceCache cache;
    return cache;
}


}