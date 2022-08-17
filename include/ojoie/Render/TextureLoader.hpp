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
/// \result graphic api texture id, if metal in apple could be id<MTLTexture>, needed to be free in a platform way
RC::Texture loadTexture(const char *path);

RC::Texture loadTextureFromMemory(const unsigned char *mem, unsigned int len);

};

}

#endif//OJOIE_TEXTURELOADER_HPP
