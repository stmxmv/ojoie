//
// Created by Aleudillonam on 9/5/2022.
//

#ifndef OJOIE_RENDERRESOURCECACHE_HPP
#define OJOIE_RENDERRESOURCECACHE_HPP

#include "Core/typedef.h"
#include "Core/SpinLock.hpp"
#include "Render/private/vulkan/FrameBuffer.hpp"
#include "Render/private/vulkan/DescriptorSetLayout.hpp"
#include "Render/private/vulkan/PipelineLayout.hpp"
#include "Render/private/vulkan/RenderPipeline.hpp"
#include "Render/private/vulkan/hash.hpp"
#include <unordered_map>
#include <mutex>

namespace AN::VK {
template<typename K, typename V>
using RenderResourceCacheStateMap = std::unordered_map<K, V>;

/// \note insertion into std::unordered_map container only invalidates iterators but not references and pointers.
///       see https://en.cppreference.com/w/cpp/container/unordered_map/insert
struct RenderResourceCacheState {

    /// \brief cached by RenderTarget and RenderPass
    RenderResourceCacheStateMap<size_t, FrameBuffer> framebuffers;

    RenderResourceCacheStateMap<size_t, RenderPass> renderPasses;

    RenderResourceCacheStateMap<size_t, DescriptorSetLayout> descriptorSetLayouts;

    RenderResourceCacheStateMap<size_t, PipelineLayout> pipelineLayouts;

    RenderResourceCacheStateMap<size_t, ShaderLibrary> shaderLibraries;

    RenderResourceCacheStateMap<size_t, ShaderFunction> shaderFunctions;

    RenderResourceCacheStateMap<size_t, RenderPipeline> renderPipelines;

    void clear() {
        framebuffers.clear();
        renderPasses.clear();
        descriptorSetLayouts.clear();
        pipelineLayouts.clear();
        shaderLibraries.clear();
        shaderFunctions.clear();
        renderPipelines.clear();
    }
};

namespace detail {

template <typename T, typename... A>
T &request_resource(Device &device, RenderResourceCacheStateMap<std::size_t, T> &resources, A &... args) {

    size_t hash{0U};
    AN::hash_param(hash, args...);

    auto res_it = resources.find(hash);

    if (res_it != resources.end()) {
        return res_it->second;
    }

    // If we do not have it already, create and cache it
    const char *res_type = typeid(T).name();
    size_t      res_id   = resources.size();

    ANLog("Building #%d cache object (%s)", res_id, res_type);

// Only error handle in release
#ifdef AN_DEBUG
    try
    {
#endif
        T resource;

        if (!resource.init(device, args...)) {
            ANLog("RenderResource Create fail");
            throw AN::Exception("RenderResource Create fail");
        }

        auto res_ins_it = resources.emplace(hash, std::move(resource));

        if (!res_ins_it.second)
        {
            throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
        }

        res_it = res_ins_it.first;

#ifdef AN_DEBUG
    } catch (const std::exception &e) {
        ANLog("Creation error for #%d cache object (%s)", res_id, res_type);
        throw e;
    }
#endif

    return res_it->second;
}


template <typename T, typename Mutex, typename... A>
T &request_resource(Device &device, Mutex &resource_mutex, RenderResourceCacheStateMap<size_t, T> &resources, A &... args) {
    std::lock_guard guard(resource_mutex);

    auto &res = request_resource(device, resources, args...);

    return res;
}

}


class RenderResourceCache : private NonCopyable {
    typedef AN::SpinLock Mutex;
//    typedef std::mutex Mutex;

    Device *_device;
    Mutex framebuffer_mutex;
    Mutex renderPass_mutex;
    Mutex descriptorSetLayout_mutex;
    Mutex pipelineLayout_mutex;
    Mutex shaderLibrary_mutex;
    Mutex shaderFunction_mutex;
    Mutex renderPipeline_mutex;

    RenderResourceCacheState state;

public:

    RenderResourceCache() = default;

    RenderResourceCache(RenderResourceCache &&other) noexcept : _device(other._device),
                                                                state(std::move(other.state)) {
        other._device = nullptr;
    }

    ~RenderResourceCache() {
        deinit();
    }

    bool init(Device &device) {
        _device = &device;
        return true;
    }

    void deinit() {
        state.clear();
    }

    FrameBuffer &newFrameBuffer(const RenderTarget &renderTarget, const RenderPass &renderPass) {
        return detail::request_resource(*_device, framebuffer_mutex, state.framebuffers, renderTarget, renderPass);
    }

    RenderPass &newRenderPass(const RenderPassDescriptor &renderPassDescriptor) {
        return detail::request_resource(*_device, renderPass_mutex, state.renderPasses, renderPassDescriptor);
    }

    DescriptorSetLayout &newDescriptorSetLayout(const DescriptorSetLayoutDescriptor &descriptorSetLayoutDescriptor) {
        return detail::request_resource(*_device, descriptorSetLayout_mutex, state.descriptorSetLayouts, descriptorSetLayoutDescriptor);
    }

    template<typename ShaderResources>
    DescriptorSetLayout &newDescriptorSetLayout(ShaderResources &&resources, bool dynamicResources) {
        return detail::request_resource(*_device, descriptorSetLayout_mutex,
                                        state.descriptorSetLayouts,
                                        std::forward<ShaderResources>(resources), dynamicResources);
    }

    template<typename Functions>
    PipelineLayout &newPipelineLayout(Functions &&funcs, bool dynamicResources) {
        return detail::request_resource(*_device, pipelineLayout_mutex,
                                        state.pipelineLayouts,
                                        std::forward<Functions>(funcs),
                                                dynamicResources);
    }

    ShaderLibrary &newShaderLibrary(const char *path) {
        return detail::request_resource(*_device, shaderLibrary_mutex, state.shaderLibraries, path);
    }

    ShaderLibrary &newShaderLibrary(const char *code, uint64_t size) {
        return detail::request_resource(*_device, shaderLibrary_mutex, state.shaderLibraries, code, size);
    }

    ShaderFunction &newShaderFunction(ShaderLibrary &library, VkShaderStageFlags stage, const char *entryPoint) {
        std::lock_guard lock(shaderFunction_mutex);
        size_t hash = 0;
        AN::hash_param(hash, &library, stage, entryPoint);

        auto found = state.shaderFunctions.find(hash);
        if (found != state.shaderFunctions.end()) {
            return found->second;
        }
        auto iter = state.shaderFunctions.insert({hash, library.newFunction(stage, entryPoint)});
        return iter.first->second;
    }

    RenderPipeline &newRenderPipeline(RC::RenderPipelineStateDescriptor &descriptor, RenderPass &renderPass) {
        return detail::request_resource(*_device, renderPipeline_mutex, state.renderPipelines, descriptor, renderPass);
    }

    void clearFrameBuffers() {
        std::lock_guard lock(framebuffer_mutex);
        state.framebuffers.clear();
    }

};

}

#endif//OJOIE_RENDERRESOURCECACHE_HPP
