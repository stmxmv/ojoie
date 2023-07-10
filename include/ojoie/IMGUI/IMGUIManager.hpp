//
// Created by aojoie on 5/17/2023.
//

#ifndef OJOIE_IMGUIMANAGER_HPP
#define OJOIE_IMGUIMANAGER_HPP

#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/Texture2D.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/IMGUI/IMGUI.hpp>
#include <ojoie/Object/ObjectPtr.hpp>

namespace AN {

class AN_API IMGUIManager {

    struct ScissorRectInfo {
        float x, y, width, height;
    };
    struct DrawCommand {
        UInt32 vertexOffset;
        UInt32 indexOffset;
        UInt32 indexCount;
        ScissorRectInfo scissor;
        Texture *tex;
    };

    float _dpiScaleX;
    Window *window;

    Shader *shader;
    ObjectPtr<Material>  material;
    ObjectPtr<Texture2D> fontTexture;

//    std::vector<uint8_t> vertex_data;
//    std::vector<uint8_t> index_data;

//    VertexBuffer vertexBuffer;

    ChannelInfoArray channelInfo;

    IMGUIList _IMGUIList;

    void loadFont();

    void renderDrawData(RenderContext &context, ImDrawData* draw_data, bool main);

public:

    bool init();

    void deinit();

    void onNewFrame();

    void updateIMGUIComponents();

    void addIMGUIComponent(IMGUIListNode &node);

    void removeIMGUIComponent(IMGUIListNode &node);

    // update render data
    void update(UInt32 frameVersion);

    void render(RenderContext &context);

    void dragEvent(float x, float y);

    bool viewportEnabled();
};

AN_API IMGUIManager &GetIMGUIManager();

}

#endif//OJOIE_IMGUIMANAGER_HPP
