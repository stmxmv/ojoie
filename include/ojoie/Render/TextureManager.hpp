//
// Created by aojoie on 5/18/2023.
//

#ifndef OJOIE_TEXTUREMANAGER_HPP
#define OJOIE_TEXTUREMANAGER_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {

class AN_API TextureManager {
public:
    virtual ~TextureManager() = default;

    virtual void update(UInt32 version) = 0;
};

AN_API TextureManager &GetTextureManager();

}

#endif//OJOIE_TEXTUREMANAGER_HPP
