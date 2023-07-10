//
// Created by aojoie on 5/25/2023.
//

#pragma once

#include <ojoie/Render/Texture2D.hpp>

namespace AN {

class AN_API TextureCube : public Texture2D {

    std::vector<Texture2D *> _sourceTextures;

    DECLARE_DERIVED_AN_CLASS(TextureCube, Texture2D)
    DECLARE_OBJECT_SERIALIZE(TextureCube)

public:

    explicit TextureCube(ObjectCreationMode mode);

    virtual bool init(const TextureDescriptor &desc,
                      const SamplerDescriptor &samplerDescriptor = DefaultSamplerDescriptor()) override;

    void setSourceTexture(UInt32 index, Texture2D *tex);

    virtual void uploadToGPU(bool generateMipmap) override;

    void buildFromSources();

};

}
