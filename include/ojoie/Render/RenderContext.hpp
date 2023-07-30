//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

#include <ojoie/Core/Exception.hpp>
#include <ojoie/Render/CommandBuffer.hpp>
#include <ojoie/Render/Layer.hpp>

namespace AN {

namespace RC {

class Scene;


}
struct GraphicContext;


struct RenderContext {
    UInt32 frameWidth, frameHeight; // the render target size (resolution)
    UInt32 layerWidth, layerHeight;
    float dpiScaleX, dpiScaleY;
    UInt32 frameVersion;
    UInt32 frameIndex;
    AN::CommandBuffer *commandBuffer; // this commandBuffer is used in render pass
};


enum GraphicsAPI {
    kGraphicsAPID3D11,
    kGraphicsAPIVulkan
};

AN_API void InitializeRenderContext(GraphicsAPI api);
AN_API void DeallocRenderContext();

AN_API GraphicsAPI GetGraphicsAPI();

/// called in any thread
AN_API void RenderContextWaitIdle();

}


#endif//OJOIE_RENDERCONTEXT_H
