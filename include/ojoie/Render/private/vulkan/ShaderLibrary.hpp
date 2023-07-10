//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_SHADERLIBRARY_HPP
#define OJOIE_VK_SHADERLIBRARY_HPP

#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/private/vulkan.hpp>
#include <SpvReflect/spirv_reflect.h>

namespace AN::VK {

class Device;

//enum class ShaderResourceType {
//    Input,
//    InputAttachment,
//    Output,
//    Image,
//    ImageSampler,
//    ImageStorage,
//    Sampler,
//    BufferUniform,
//    BufferStorage,
//    PushConstant,
//    SpecializationConstant,
//    All
//};
//
//struct ShaderResource {
//    VkShaderStageFlags stages;
//    ShaderResourceType type;
//    uint32_t set;
//    uint32_t binding;
//    uint32_t location;
//    uint32_t input_attachment_index;
//    uint32_t vec_size;
//    uint32_t columns;
//    uint32_t array_size;
//    uint32_t offset;
//    uint32_t size;
//    uint32_t constant_id;
//    bool dynamic;
//    std::string name;
//};
//
//
//class ShaderLibrary;
//
//class ShaderFunction {
//    std::string _entryPoint;
//    ShaderLibrary *_library;
//    VkShaderStageFlags _stage;
//    std::vector<ShaderResource> resources;
//    friend class ShaderLibrary;
//
//    bool init(ShaderLibrary &library, VkShaderStageFlags stage, const char *entryPoint);
//
//public:
//
//    ShaderLibrary &getLibrary() const {
//        return *_library;
//    }
//
//    VkShaderStageFlags getStageFlags() const {
//        return _stage;
//    }
//
//    const std::vector<ShaderResource> &getShaderResources() const {
//        return resources;
//    }
//
//    const char *getEntryPoint() const {
//        return _entryPoint.c_str();
//    }
//
//};
//
//class ShaderLibrary : private NonCopyable {
//    VkShaderModule module{};
//    Device *_device;
//    SpvReflectShaderModule *reflection{};
//
//    friend class ShaderFunction;
//public:
//
//    ShaderLibrary() = default;
//
//    ShaderLibrary(ShaderLibrary &&other) noexcept : module(other.module), _device(other._device), reflection(other.reflection) {
//        other.module = nullptr;
//        other.reflection = nullptr;
//    }
//
//    ~ShaderLibrary() {
//        deinit();
//    }
//
//    bool init(Device &device, const char *path);
//
//    bool init(Device &device, const void *code, uint64_t size);
//
//    void deinit();
//
//    VkShaderModule vkShaderModule() const {
//        return module;
//    }
//
//    ShaderFunction newFunction(VkShaderStageFlags stage, const char *entryPoint) {
//        ShaderFunction function{};
//        if (function.init(*this, stage, entryPoint)) {
//            return function;
//        }
//        return {};
//    }
//
//};



}

#endif//OJOIE_VK_SHADERLIBRARY_HPP
