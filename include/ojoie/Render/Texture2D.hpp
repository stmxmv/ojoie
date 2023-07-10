//
// Created by aojoie on 4/20/2023.
//

#ifndef OJOIE_TEXTURE2D_HPP
#define OJOIE_TEXTURE2D_HPP

#include <ojoie/Render/Texture.hpp>

namespace AN {

class AN_API Texture2D : public Texture {

protected:

    struct TextureData {
        UInt8      *data;
        size_t      size;
        UInt32      width, height;
        UInt32      mipmapLevel;
        PixelFormat pixelFormat;
    };

    bool bIsReadable;
    bool bUploadToGPU;
    bool bSizeChanged;

    TextureData	_texData;
    SamplerDescriptor _samplerDescriptor;

    DECLARE_DERIVED_AN_CLASS(Texture2D, Texture)
    DECLARE_OBJECT_SERIALIZE(Texture2D)

    virtual bool init() override;

public:

    explicit Texture2D(ObjectCreationMode mode);

    /// TextureDescriptor set the format and initial size of the texture2D
    virtual bool init(const TextureDescriptor &desc,
                      const SamplerDescriptor &samplerDescriptor = DefaultSamplerDescriptor());

    virtual bool initAfterDecode() override;

    virtual void dealloc() override;

    virtual void uploadToGPU(bool generateMipmap = false);

    /// texture2D default is not readable
    void setReadable(bool readable) { bIsReadable = readable; }
    bool isReadable() const { return bIsReadable; }

    UInt32 getDataWidth() const { return _texData.width; }
    UInt32 getDataHeight() const { return _texData.height; }

    UInt32 getDataSize() const;

    /// resize will cause uploaded GPU texture to become invalid
    void resize(UInt32 width, UInt32 height);

    void setPixelData(const UInt8 *data);

    void *getPixelData() const { return _texData.data; }

    PixelFormat getPixelFormat() const { return _texData.pixelFormat; }

};


}

#endif//OJOIE_TEXTURE2D_HPP
