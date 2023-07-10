//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_FONT_HPP
#define OJOIE_FONT_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/Render/RenderPipelineState.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/Sampler.hpp>
#include <ojoie/Core/CGTypes.hpp>
#include <unordered_map>
namespace AN {


struct character_info {
    float ax; // advance.x
    float ay; // advance.y

    float bw; // bitmap.width;
    float bh; // bitmap.rows;

    float bl; // bitmap_left;
    float bt; // bitmap_top;

    float tx; // x offset of glyph in texture coordinates
    float ty;
};

class FontAtlas {
//    RC::Texture tex;// texture object

    unsigned int width; // width of texture in pixels
    unsigned int height;// height of texture in pixels

    unsigned int maxFontHeight;
    unsigned int maxUnderBaseline;

    std::unordered_map<unsigned long, character_info> characters;

    inline constexpr static int max_width = 1024;

    friend class FontManager;
public:

    ~FontAtlas() {
        deinit();
    }

    bool init(const char *fontFile, unsigned long charStart, unsigned long charEnd, int fontSize);

    void deinit();

    bool contains(unsigned long ch) const {
        return characters.contains(ch);
    }

    const character_info &getInfo(unsigned long ch) const {
        return characters.at(ch);
    }

    unsigned int getWidth() const { return width; }

    unsigned int getHeight() const { return height; }

    unsigned int getMaxFontHeight() const { return maxFontHeight; }

    unsigned int getMaxUnderBaseline() const { return maxUnderBaseline; }
};

/// \RenderActor
class FontManager {

    struct vertex {
        float x, y, s, t;
    };

    struct DrawCommandInfo {
        float width;
        float edge;

        Math::vec4 textColor;

        uint64_t vertexCount;
        uint64_t vertexOffset;

        FontAtlas *atlas;
    };

    std::vector<vertex> vertices;
    std::vector<DrawCommandInfo> drawCommands;

    FontAtlas defaultFontAtlas;

    RenderPipelineState renderPipelineState;
    RC::Sampler sampler;

public:

    inline constexpr static int DefaultFontSize = 48;

    static FontManager &GetSharedManager();

    bool init();

    void deinit();

    void renderText(const char * text, const Math::vec4 &color, float width , float edge, float x, float y, float sx, float sy);

    void renderText(const FontAtlas &atlas, const char *text, const Math::vec4 &color, float width ,float edge, float x, float y, float sx, float sy);

    // unicode 32
    void renderText(const FontAtlas &atlas, const unsigned long *text, const Math::vec4 &color, float width , float edge, float x, float y, float sx, float sy);


    const FontAtlas &getDefaultFontAtlas() const {
        return defaultFontAtlas;
    }

    template<typename Char>
    Size CalTextSize(const Char *text, float sx, float sy, const FontAtlas &atlas) {
        const Char *p;

        Size size{};
        float cursorY = 0.f;
        for (p = text; *p; p++) {
            /* Calculate the vertex and texture coordinates */
            unsigned long ch = *p;
            if (!atlas.contains(ch)) {
                ch = '?';
            }
            const auto &character = atlas.getInfo(ch);

            float w  = character.ax * sx;;

            cursorY += character.ay * sy;
            float h  = character.bh * sy + cursorY;

            size.width += w;
            size.height = std::max(size.height, (double)h);
        }

        return size;
    }

    template<typename Char>
    Size CalTextSize(const Char *text, float sx, float sy) {
        return CalTextSize(text, sx, sy, defaultFontAtlas);
    }

    void renderFrame();
};

inline FontManager &GetFontManager() {
    return FontManager::GetSharedManager();
}

class FontManagerNode : public Node {
    typedef FontManagerNode Self;
    typedef Node Super;
public:

    struct FontManagerNodeSceneProxy : Super::SceneProxyType {

        explicit FontManagerNodeSceneProxy(FontManagerNode &node) : Super::SceneProxyType(node) {}

        virtual void postRender(const RenderContext &context) override {
            Super::SceneProxyType::postRender(context);
            GetFontManager().renderFrame();
        }
    };

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    FontManagerNode() {
        _postRender = true;
    }


    virtual RC::SceneProxy *createSceneProxy() override {
        return new FontManagerNodeSceneProxy(*this);
    }


};

}

#endif//OJOIE_FONT_HPP
