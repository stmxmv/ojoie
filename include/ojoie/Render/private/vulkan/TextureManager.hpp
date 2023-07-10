//
// Created by aojoie on 4/20/2023.
//

#ifndef OJOIE_VK_TEXTUREMANAGER_HPP
#define OJOIE_VK_TEXTUREMANAGER_HPP

#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Render/private/vulkan/Buffer.hpp>
#include <ojoie/Render/private/vulkan/Semaphore.hpp>
#include <ojoie/Threads/Threads.hpp>
#include <ojoie/Utility/Utility.h>
#include <ojoie/Render/TextureManager.hpp>

#include <shared_mutex>
#include <unordered_map>

namespace AN::VK {

struct SamplerDescriptorHasher {
    size_t operator() (const SamplerDescriptor &desc) const {
        size_t result = 0;
        Math::hash_combine(result, desc.normalizedCoordinates);
        Math::hash_combine(result, desc.addressModeU);
        Math::hash_combine(result, desc.addressModeV);
        Math::hash_combine(result, desc.addressModeW);
        Math::hash_combine(result, desc.borderColor);
        Math::hash_combine(result, desc.filter);
        Math::hash_combine(result, desc.maxAnisotropy);
        Math::hash_combine(result, desc.compareFunction);
        Math::hash_combine(result, desc.lodMinClamp);
        Math::hash_combine(result, desc.lodMaxClamp);
        return result;
    }
};


struct Texture {
    bool              pendingDestroy = false;
    VkImage           image;
    VmaAllocation     imageAllocation;
    VkImageView       imageView;
    SamplerDescriptor samplerDescriptor;
};


struct StagingTextureBuffer {
    Buffer    buffer;
    Semaphore semaphore;
    UInt64    lastSemaphoreValue;
};

/// use std::unique_ptr to avoid rehashing cause pointer change
typedef std::unordered_map<TextureID, std::unique_ptr<Texture>> TextureIDMap;
typedef std::vector<StagingTextureBuffer>                       StagingTextureBuffers;
typedef std::unordered_map<SamplerDescriptor,
                           VkSampler,
                           SamplerDescriptorHasher,
                           memcmp_equal<SamplerDescriptor>>
        SamplerMap;

/// \brief manage all textures in game, thread safe
/// TextureManager don't need to init, but need to dealloc all its allocated resources after used
class TextureManager : public AN::TextureManager {

    TextureIDMap textureIdMap;
    SamplerMap   samplerMap;

    std::unordered_map<ThreadID,
                       std::unique_ptr<StagingTextureBuffers>>
            threadStagingTextureBuffers;

    std::shared_mutex threadStagingTextureBuffers_mutex;
    std::shared_mutex textureIdMap_mutex;
    std::shared_mutex samplerMap_mutex;

    struct PendingInfo {
        TextureID id;
        Texture tex;
        UInt32 version;
    };
    std::vector<PendingInfo> _pendingDestroyTextures;

    UInt32 _currentVersion;

public:
    ~TextureManager();

    /// upload can be multi-threaded
    /// currently only support mipLevel == 1
    void uploadTexture2DData(const UInt8 *srcData, VkImage image,
                             UInt32 width, UInt32 height,
                             PixelFormat format,
                             UInt32      mipLevel);

    void uploadTexture2D(TextureID tid, const UInt8 *srcData,
                         UInt32 width, UInt32 height,
                         PixelFormat format, bool generateMipmap,
                         const SamplerDescriptor &samplerDescriptor);

    void registerRenderTarget(TextureID id, const Texture &tex);

    /// \nullable
    Texture *getTexture(TextureID id);

    /// \nullable
    VkSampler getSampler(const SamplerDescriptor &desc);

    /// not thread safe
    void deallocTexture(TextureID id);

    /// deallocate all the resources this manager allocated not thread safe
    void deallocTextureResources();

    void pendingDestroyTextures();

    virtual void update(UInt32 version) override;
};

TextureManager &GetTextureManager();

}// namespace AN::VK

#endif//OJOIE_VK_TEXTUREMANAGER_HPP
