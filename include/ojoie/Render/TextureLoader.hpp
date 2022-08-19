//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_TEXTURELOADER_HPP
#define OJOIE_TEXTURELOADER_HPP

#include <ojoie/Render/Texture.hpp>
#include <cstdint>
namespace AN {

namespace TextureLoader {

/// \RenderActor
RC::Texture loadTexture(const char *path, bool sRgb = true);

RC::Texture loadTextureFromMemory(const unsigned char *mem, unsigned int len, bool sRgb = true);

};

}

#endif//OJOIE_TEXTURELOADER_HPP
