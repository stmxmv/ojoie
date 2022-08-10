//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_FONT_HPP
#define OJOIE_FONT_HPP

#include <ojoie/Render/RenderPipeline.hpp>
#include <ojoie/Math/Math.hpp>

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
    uint64_t tex{};// texture object

    unsigned int width; // width of texture in pixels
    unsigned int height;// height of texture in pixels

    std::unordered_map<unsigned long, character_info> characters;

    inline constexpr static int max_width = 1024;

    friend class FontManager;
public:

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


};

/// \RenderActor
class FontManager {
    struct Impl;
    Impl *impl;

    bool defaultFontAtlasInited{};
    FontAtlas defaultFontAtlas;

    void beforeRenderText(const FontAtlas &atlas);

    void doRenderText(uint64_t count);

    FontManager();

    ~FontManager();

public:

    inline constexpr static int DefaultFontSize = 48;

    static FontManager &GetSharedManager();

    bool init();

    void deinit();

    void renderText(RenderPipeline &pipeline,const char * text, float x, float y, float sx, float sy);

    void renderText(RenderPipeline &pipeline, const FontAtlas &atlas, const char *text, float x, float y, float sx, float sy);

    // unicode 32
    void renderText(RenderPipeline &pipeline, const FontAtlas &atlas, const unsigned long *text, float x, float y, float sx, float sy);
};

inline FontManager &GetFontManager() {
    return FontManager::GetSharedManager();
}

}

#endif//OJOIE_FONT_HPP
