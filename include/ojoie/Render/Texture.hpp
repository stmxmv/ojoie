//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_TEXTURE_H
#define OJOIE_TEXTURE_H

#include <ojoie/Object/NamedObject.hpp>
#include <ojoie/Render/RenderTypes.hpp>

namespace AN {

typedef UInt64 TextureID;

struct TextureDescriptor {
    UInt32      width, height;
    PixelFormat pixelFormat;
    UInt32      mipmapLevel;
};

class AN_API Texture : public NamedObject {

    TextureID _textureID;

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Texture, NamedObject)

public:

    explicit Texture(ObjectCreationMode mode);

    virtual bool init() override;
    virtual bool initAfterDecode() override;

    TextureID getTextureID() const { return _textureID; }

    static const SamplerDescriptor &DefaultSamplerDescriptor();
};


}// namespace AN

#endif//OJOIE_TEXTURE_H
