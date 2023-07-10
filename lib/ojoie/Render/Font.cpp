//
// Created by Aleudillonam on 8/8/2022.
//

#include <ojoie/Utility/Log.h>
#include "Render/Font.hpp"
#include "Render/Sampler.hpp"

#include "Render/Renderer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <optional>

namespace AN {

struct FontPushConstantObject {
    alignas(4) float width;
    alignas(4) float edge;
    alignas(16) Math::vec4 textColor;
    alignas(16) Math::mat4 projection;
};

FontManager &FontManager::GetSharedManager() {
    static FontManager fontManager;
    return fontManager;
}

void FontManager::deinit() {

    defaultFontAtlas.deinit();

    renderPipelineState.deinit();
    sampler.deinit();
}

bool FontManager::init() {

//    RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();
//
//    samplerDescriptor.addressModeU = RC::SamplerAddressMode::ClampToEdge;
//    samplerDescriptor.addressModeV = RC::SamplerAddressMode::ClampToEdge;

//    if (!sampler.init(samplerDescriptor)) {
//        return false;
//    }

    const RenderContext &context = GetRenderer().getRenderContext();

    VertexDescriptor vertexDescriptor{};
    vertexDescriptor.attributes[0].format = kChannelFormatFloat;
    vertexDescriptor.attributes[0].dimension = 4;
    vertexDescriptor.attributes[0].binding = 0;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].location = 0;

    vertexDescriptor.layouts[0].stepFunction = kVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = sizeof(vertex);

    DepthStencilDescriptor depthStencilDescriptor{};
    depthStencilDescriptor.depthTestEnabled = false;
    depthStencilDescriptor.depthWriteEnabled = false;
    depthStencilDescriptor.depthCompareFunction = kCompareFunctionNever;

    RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
    throw std::runtime_error("not implement");
//    renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "text.vert.spv" };
//    renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "text.frag.spv" };

    renderPipelineStateDescriptor.colorAttachments[0].writeMask = kColorWriteMaskAll;
    renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;

    renderPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = kBlendFactorSourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = kBlendFactorOneMinusSourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = kBlendOperationAdd;

    renderPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = kBlendFactorZero;
    renderPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = kBlendFactorOne;
    renderPipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = kBlendOperationAdd;


//    renderPipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
//    renderPipelineStateDescriptor.depthStencilDescriptor = depthStencilDescriptor;

    renderPipelineStateDescriptor.rasterSampleCount = context.msaaSamples;
    renderPipelineStateDescriptor.alphaToOneEnabled = false;
    renderPipelineStateDescriptor.alphaToCoverageEnabled = false;


    renderPipelineStateDescriptor.cullMode = kCullModeNone;

//    if (!renderPipelineState.init(std::move(renderPipelineStateDescriptor))) {
//        return false;
//    }

    float scale = context.frameWidth / 1920.f;

    return defaultFontAtlas.init("C:\\Windows\\Fonts\\arial.ttf", 0, 128, DefaultFontSize * scale);
}


void FontManager::renderText(const char *text, const Math::vec4 &color, float width , float edge, float x, float y, float sx, float sy) {
    renderText(defaultFontAtlas, text, color, width, edge, x, y, sx, sy);
}

template<typename Vertices, typename Char>
static uint64_t prepareVertexBuffer(Vertices &vertices, const FontAtlas &atlas, const Char *text, float x, float y, float sx, float sy) {
    int text_len;

    if constexpr (std::is_same_v<Char, char>) {

        text_len = (int)strlen(text);

    } else {
        text_len = 0;
        while (text[text_len]) {
            ++text_len;
        }
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
        vertices[c++] = { x2,         -y2, character.tx, character.ty };
        vertices[c++] = { x2,     -y2 - h, character.tx, character.ty + character.bh / (float)atlas.getHeight() };
        vertices[c++] = { x2 + w,     -y2, character.tx + character.bw / (float)atlas.getWidth(), character.ty };
        vertices[c++] = { x2 + w,     -y2, character.tx + character.bw / (float)atlas.getWidth(), character.ty };
        vertices[c++] = { x2,     -y2 - h, character.tx, character.ty + character.bh / (float)atlas.getHeight() };
        vertices[c++] = { x2 + w, -y2 - h, character.tx + character.bw / (float)atlas.getWidth(), character.ty + character.bh / (float)atlas.getHeight() };
        // clang-format on
    }

    return c;
}


void FontManager::renderFrame() {

    if (drawCommands.empty()) {
        return;
    }

    const RenderContext &context = GetRenderer().getRenderContext();

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;

    renderCommandEncoder.setRenderPipelineState(renderPipelineState);

    renderCommandEncoder.bindSampler(0, sampler);


    Math::mat4 projectionMatrix = Math::ortho(0.0f, context.frameWidth,  context.frameHeight, 0.f, -1.0f, 1.0f);


    int64_t buffer_size = (int64_t)(vertices.size() * sizeof(vertex));

    RC::BufferAllocation vertexBufferAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::VertexBuffer, buffer_size);

    memcpy(vertexBufferAllocation.map(), vertices.data(), buffer_size);

    vertexBufferAllocation.getBuffer().flush();

    for (auto &command : drawCommands) {
//        renderCommandEncoder.bindTexture(2, (RC::Texture &)command.atlas->tex);

        renderCommandEncoder.bindVertexBuffer(0, vertexBufferAllocation.getOffset() + command.vertexOffset * sizeof(vertex),
                                              vertexBufferAllocation.getBuffer());

        FontPushConstantObject pc;
        pc.textColor = command.textColor;
        pc.edge = command.edge;
        pc.width = command.width;
        pc.projection = projectionMatrix;

        renderCommandEncoder.pushConstants(0, sizeof(FontPushConstantObject), &pc);

        renderCommandEncoder.draw(command.vertexCount);
    }


    drawCommands.clear();
    vertices.clear();
}

void FontManager::renderText(const FontAtlas &atlas, const char *text, const Math::vec4 &color, float width , float edge,float x, float y, float sx, float sy) {
    std::vector<vertex> newVertices;
    auto count = prepareVertexBuffer(newVertices, atlas, text, x, y, sx, sy);

    drawCommands.push_back(
            DrawCommandInfo {
                    .width = width, .edge = edge, .textColor = color,
                    .vertexCount = count, .vertexOffset = vertices.size(),
                    .atlas = const_cast<FontAtlas *>(&atlas)
            }
    );

    vertices.insert(vertices.end(), newVertices.begin(), newVertices.begin() + count);
}

void FontManager::renderText(const FontAtlas &atlas, const unsigned long *text, const Math::vec4 &color, float width , float edge, float x, float y, float sx, float sy) {
    std::vector<vertex> newVertices;
    auto count = prepareVertexBuffer(newVertices, atlas, text, x, y, sx, sy);

    drawCommands.push_back(
            DrawCommandInfo {
                    .width = width, .edge = edge, .textColor = color,
                    .vertexCount = count, .vertexOffset = vertices.size(),
                    .atlas = const_cast<FontAtlas *>(&atlas)
            }
    );

    vertices.insert(vertices.end(), newVertices.begin(), newVertices.begin() + count);
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

    maxUnderBaseline = 0;
    maxFontHeight = 0;

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


        maxFontHeight = std::max(maxFontHeight, g->bitmap.rows);
        if (g->bitmap.rows > g->bitmap_top) {
            maxUnderBaseline = std::max(maxUnderBaseline, g->bitmap.rows - g->bitmap_top);
        }

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
//    RC::TextureDescriptor textureDescriptor{};
//    textureDescriptor.pixelFormat = RC::PixelFormat::R8Unorm;
//    textureDescriptor.width = width;
//    textureDescriptor.height = height;
//    textureDescriptor.mipmapLevel = 1;
//
//    if (!tex.init(textureDescriptor)) {
//        return false;
//    }

    /* Paste all glyph bitmaps into the texture, remembering the offset */
    unsigned int ox = 0;
    unsigned int oy = 0;

    rowh = 0;

    const RenderContext &context = GetRenderer().getRenderContext();


//    RC::BufferAllocation bufferAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::TransferSource,
//                                                                         (uint64_t)textureDescriptor.width * textureDescriptor.height);


    uint64_t stageOffset = 0;
//    void *stageBufferData = bufferAllocation.map();

    RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

    for (const auto &info : bitmap_infos) {
        unsigned long i = info.ch;

        if (ox + info.width + 1 >= max_width) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }
//
//        if (info.width != 0 && info.rows != 0) {
//            memcpy((char *)stageBufferData + stageOffset, info.buffer.data(), (size_t)info.width * info.rows);
//            blitCommandEncoder.copyBufferToTexture(bufferAllocation.getBuffer(),
//                                                   bufferAllocation.getOffset() + stageOffset, 0, 0, tex, 0, ox, oy, info.width, info.rows);
//            stageOffset += (uint64_t)info.width * info.rows;
//        }

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

//    bufferAllocation.getBuffer().flush();



    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    ANLog("FontAtlas generated a %d x %d (%d kb) texture atlas", width, height, width * height / 1024);

    return true;
}

void FontAtlas::deinit() {
//    tex.deinit();
}
}// namespace AN