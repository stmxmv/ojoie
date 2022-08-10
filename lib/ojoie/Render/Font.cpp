//
// Created by Aleudillonam on 8/8/2022.
//

#include "Render/Font.hpp"
#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace AN {



struct vertex {
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};

struct FontManager::Impl {

    unsigned int vao, vbo;

    uint64_t bufferSize{};

    std::vector<vertex> vertices;
    
};

FontManager::FontManager() : impl(new Impl{}) {}

FontManager::~FontManager() {
    delete impl;
}

FontManager &FontManager::GetSharedManager() {
    static FontManager fontManager;
    return fontManager;
}

void FontManager::deinit() {

    if (defaultFontAtlasInited) {
        defaultFontAtlas.deinit();
    }

    glDeleteVertexArrays(1, &impl->vao);
    glDeleteBuffers(1, &impl->vbo);
}

bool FontManager::init() {

    /// init vertex buffer
    glGenVertexArrays(1, &impl->vao);
    glGenBuffers(1, &impl->vbo);
    glBindVertexArray(impl->vao);

    return true;
}

void FontManager::renderText(RenderPipeline &pipeline, const char *text, float x, float y, float sx, float sy) {
    if (!defaultFontAtlasInited) {
        defaultFontAtlasInited = true;
        defaultFontAtlas.init("C:\\Windows\\Fonts\\arial.ttf", 0, 128, DefaultFontSize);
    }
    renderText(pipeline, defaultFontAtlas, text, x, y, sx, sy);
}

template<typename Vertices, typename Char>
static uint64_t prepareVertexBuffer(Vertices &vertices, const FontAtlas &atlas, const Char *text, float x, float y, float sx, float sy) {
    int text_len = 0;
    while (text[text_len]) {
        ++text_len;
    }

    vertices.resize(6 * text_len);
    uint64_t c = 0;
    const Char *p;
    for (p = text; *p; p++) {
        /* Calculate the vertex and texture coordinates */
        unsigned long ch = *p;
        if (!atlas.contains(ch)) {
            ch = '?';
        }
        const auto &character = atlas.getInfo(ch);

        float x2 = x + character.bl * sx;
        float y2 = -y - character.bt * sy;
        float w  = character.bw * sx;
        float h  = character.bh * sy;

        /* Advance the cursor to the start of the next character */
        x += character.ax * sx;
        y += character.ay * sy;

        /* Skip glyphs that have no pixels */
        if (w == 0 || h == 0) {
            continue;
        }
        // clang-format off
        vertices[c++] = vertex{ x2,         -y2, character.tx, character.ty };
        vertices[c++] = vertex{ x2,     -y2 - h, character.tx, character.ty + character.bh / (float)atlas.getHeight() };
        vertices[c++] = vertex{ x2 + w,     -y2, character.tx + character.bw / (float)atlas.getWidth(), character.ty };
        vertices[c++] = vertex{ x2 + w,     -y2, character.tx + character.bw / (float)atlas.getWidth(), character.ty };
        vertices[c++] = vertex{ x2,     -y2 - h, character.tx, character.ty + character.bh / (float)atlas.getHeight() };
        vertices[c++] = vertex{ x2 + w, -y2 - h, character.tx + character.bw / (float)atlas.getWidth(), character.ty + character.bh / (float)atlas.getHeight() };
        // clang-format on
    }

    return c;
}

void FontManager::beforeRenderText(const FontAtlas &atlas) {
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(impl->vao);

    glBindTexture(GL_TEXTURE_2D, atlas.tex);

}
void FontManager::doRenderText(uint64_t count) {
    glBindBuffer(GL_ARRAY_BUFFER, impl->vbo);

    int64_t buffer_size = (int64_t)(impl->vertices.size() * sizeof(vertex));
    if (impl->bufferSize > buffer_size) {

        glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, impl->vertices.data());

    } else {
        /// regenerate the vertex buffer
        impl->bufferSize = buffer_size;
        glBufferData(GL_ARRAY_BUFFER, buffer_size, impl->vertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    }


    glDrawArrays(GL_TRIANGLES, 0, count);

    glDepthMask(GL_TRUE);
}

void FontManager::renderText(RenderPipeline &pipeline, const FontAtlas &atlas, const char *text, float x, float y, float sx, float sy) {
    beforeRenderText(atlas);
    auto count = prepareVertexBuffer(impl->vertices, atlas, text, x, y, sx, sy);
    doRenderText(count);
}

void FontManager::renderText(RenderPipeline &pipeline, const FontAtlas &atlas, const unsigned long *text, float x, float y, float sx, float sy) {
    beforeRenderText(atlas);
    auto count = prepareVertexBuffer(impl->vertices, atlas, text, x, y, sx, sy);
    doRenderText(count);
}

bool FontAtlas::init(const char *fontFile, unsigned long charStart, unsigned long charEnd, int fontSize) {
    characters.clear();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        ANLog("ERROR::FREETYPE: Could not init FreeType Library");
        return false;
    }

    FT_Face face;

#ifdef _WIN32
    if (FT_New_Face(ft, fontFile, 0, &face)) {
        ANLog("ERROR::FREETYPE: Failed to load font");
        return false;
    }

#else

    /// TODO other platfrom font location



#endif

    FT_Set_Pixel_Sizes(face, 0, fontSize);


    unsigned int roww = 0;
    unsigned int rowh = 0;
    width = 0;
    height = 0;

    struct bitmap_info {
        unsigned long ch;

        struct {
            long x;
            long y;
        } advance;

        unsigned int width;
        unsigned int rows;

        int left;
        int top;

        std::vector<unsigned char> buffer;
    };

    std::vector<bitmap_info> bitmap_infos;

    /* Find minimum size for a texture holding all visible ASCII characters */
    for (unsigned long i = charStart; i < charEnd; i++) {

        if (FT_Load_Char(face, i, FT_LOAD_DEFAULT)) {
            ANLog("Loading character %lu failed!", i);
            continue;
        }

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF)) {
            ANLog("failed to render glyph of character %lu", i);
            continue;
        }

        FT_GlyphSlot g = face->glyph;

        if (roww + g->bitmap.width + 1 >= max_width) {
            width = std::max(width, roww);
            height += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += g->bitmap.width + 1;
        rowh = std::max(rowh, g->bitmap.rows);

        /// copy generated bitmap
        bitmap_info info;
        info.ch = i;
        info.width = g->bitmap.width;
        info.rows = g->bitmap.rows;
        info.left = g->bitmap_left;
        info.top = g->bitmap_top;
        info.advance.x = g->advance.x;
        info.advance.y = g->advance.y;

        info.buffer.resize(g->bitmap.width * g->bitmap.rows);

        memcpy(info.buffer.data(), g->bitmap.buffer, g->bitmap.width * g->bitmap.rows);

        bitmap_infos.push_back(std::move(info));
    }

    width = std::max(width, roww);
    height += rowh;

    /* Create a texture that will be used to hold all ASCII glyphs */
    glActiveTexture(GL_TEXTURE0);
    unsigned int id;
    glGenTextures(1, &id);
    tex = id;
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    /* We require 1 byte alignment when uploading texture data */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* Clamping to edges is important to prevent artifacts when scaling */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Linear filtering usually looks best for text */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Paste all glyph bitmaps into the texture, remembering the offset */
    unsigned int ox = 0;
    unsigned int oy = 0;

    rowh = 0;

    for (const auto &info : bitmap_infos) {
        unsigned long i = info.ch;

        if (ox + info.width + 1 >= max_width) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, info.width, info.rows, GL_RED, GL_UNSIGNED_BYTE, info.buffer.data());
        characters[i].ax = info.advance.x >> 6;
        characters[i].ay = info.advance.y >> 6;

        characters[i].bw = info.width;
        characters[i].bh = info.rows;

        characters[i].bl = info.left;
        characters[i].bt = info.top;

        characters[i].tx = ox / (float)width;
        characters[i].ty = oy / (float)height;

        rowh = std::max(rowh, info.rows);
        ox += info.width + 1;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore alignment

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    ANLog("FontAtlas generated a %d x %d (%d kb) texture atlas", width, height, width * height / 1024);

    return true;
}

void FontAtlas::deinit() {
    if (tex) {
        GLuint id = tex;
        glDeleteTextures(1, &id);
        tex = 0;
    }
}
}// namespace AN