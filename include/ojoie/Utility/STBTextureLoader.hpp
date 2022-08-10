//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_STBTEXTURELOADER_HPP
#define OJOIE_STBTEXTURELOADER_HPP

#include <cstdint>
namespace AN {

namespace STBTextureLoader {

/// \RenderActor
/// \result graphic api texture id, if metal in apple could be id<MTLTexture>, needed to be free in a platform way
uint64_t loadTexture(const char *path);

uint64_t loadTextureFromMemory(const unsigned char *mem, unsigned int len);

};

}

#endif//OJOIE_STBTEXTURELOADER_HPP
