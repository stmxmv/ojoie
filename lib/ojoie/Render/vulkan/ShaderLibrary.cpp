//
// Created by aojoie on 9/20/2022.
//

#include "Render/private/vulkan/ShaderLibrary.hpp"
#include "Render/private/vulkan/Device.hpp"

#include <filesystem>
#include <fstream>

namespace AN::VK {

//static std::vector<char> readFile(const char *filename) {
//    std::ifstream file(filename, std::ios::ate | std::ios::binary);
//
//    if (!file.is_open()) {
//        ANLog("failed to open file!");
//        return {};
//    }
//
//    size_t fileSize = (size_t) file.tellg();
//    std::vector<char> buffer(fileSize);
//
//    file.seekg(0);
//    file.read(buffer.data(), (long long)fileSize);
//
//    file.close();
//
//    return buffer;
//}
//
//bool ShaderLibrary::init(Device &device, const char *path) {
//    std::vector<char> code = readFile(path);
//    return init(device, code.data(), code.size());
//}
//
//bool ShaderLibrary::init(Device &device, const void *code, uint64_t size) {
//    _device = &device;
//    VkShaderModuleCreateInfo createInfo{};
//    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    createInfo.codeSize = size;
//    createInfo.pCode = (const uint32_t *)code;
//
//    if (VkResult result = vkCreateShaderModule(device.vkDevice(), &createInfo, nullptr, &module);
//        result != VK_SUCCESS) {
//        ANLog("failed to create shader module, result %s", ResultCString(result));
//        return false;
//    }
//
//    reflection = (SpvReflectShaderModule *)malloc(sizeof(SpvReflectShaderModule));
//    memset(reflection, 0, sizeof(SpvReflectShaderModule));
//
//    if (SpvReflectResult result = spvReflectCreateShaderModule(size, code, reflection);
//        result != SPV_REFLECT_RESULT_SUCCESS) {
//        ANLog("failed to create shader module reflection code %d", result);
//        return false;
//    }
//
//    return true;
//}
//
//void ShaderLibrary::deinit() {
//    if (module) {
//        vkDestroyShaderModule(_device->vkDevice(), module, nullptr);
//        module = nullptr;
//    }
//    if (reflection) {
//        spvReflectDestroyShaderModule(reflection);
//        free(reflection);
//        reflection = nullptr;
//    }
//}
//
//bool ShaderFunction::init(ShaderLibrary &library, VkShaderStageFlags stage, const char *entryPoint) {
//    _library = &library;
//    _stage = stage;
//    _entryPoint = entryPoint;
//#define CHECK_RESULT(statement)                                                              \
//    do {                                                                                     \
//        if (SpvReflectResult result = (statement); SPV_REFLECT_RESULT_SUCCESS != result) {   \
//            ANLog("%s return result code %d %s %d", #statement, result, __FILE__, __LINE__); \
//            return false;                                                                    \
//        }                                                                                    \
//    } while (0)
//
//
//    /// reflect descriptor sets
//    uint32_t count;
//    std::vector<SpvReflectDescriptorSet *>  descriptorSets;
//    CHECK_RESULT(spvReflectEnumerateEntryPointDescriptorSets(library.reflection, entryPoint, &count, nullptr));
//    descriptorSets.resize(count);
//    CHECK_RESULT(spvReflectEnumerateEntryPointDescriptorSets(library.reflection, entryPoint, &count, descriptorSets.data()));
//
//    /// TODO currently make resources available to both vertex and fragment stage, future may figure it out in the pipeline
//    for (SpvReflectDescriptorSet *set : descriptorSets) {
//        for (int i = 0; i < set->binding_count; ++i) {
//            ShaderResource shaderResource{};
//
//            SpvReflectDescriptorBinding *binding = set->bindings[i];
//
//            shaderResource.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//            shaderResource.set = set->set;
//            shaderResource.binding = binding->binding;
//            shaderResource.array_size = binding->count;
//            shaderResource.name = binding->name;
//
//            switch (binding->descriptor_type) {
//                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
//                    shaderResource.type = ShaderResourceType::Sampler;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
//                    shaderResource.type = ShaderResourceType::ImageSampler;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
//                    shaderResource.type = ShaderResourceType::Image;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
//                    shaderResource.type = ShaderResourceType::ImageStorage;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
//                    shaderResource.type = ShaderResourceType::BufferUniform;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
//                    shaderResource.type = ShaderResourceType::BufferStorage;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
//                    shaderResource.type = ShaderResourceType::BufferUniform;
//                    shaderResource.dynamic = true;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
//                    shaderResource.type = ShaderResourceType::BufferStorage;
//                    shaderResource.dynamic = true;
//                    break;
//                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
//                    shaderResource.type = ShaderResourceType::InputAttachment;
//                    break;
//                default:
//                    continue;
//            }
//
//            resources.push_back(shaderResource);
//        }
//    }
//
//    /// reflect pushConstant
//    std::vector<SpvReflectBlockVariable *> blocks;
//    CHECK_RESULT(spvReflectEnumerateEntryPointPushConstantBlocks(library.reflection, entryPoint, &count, nullptr));
//    blocks.resize(count);
//    CHECK_RESULT(spvReflectEnumerateEntryPointPushConstantBlocks(library.reflection, entryPoint, &count, blocks.data()));
//
//    for (SpvReflectBlockVariable *block : blocks) {
//        ShaderResource shaderResource{};
//        shaderResource.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//        shaderResource.name = block->name;
//        shaderResource.type = ShaderResourceType::PushConstant;
//        shaderResource.offset = block->offset;
//        shaderResource.size = block->size;
//        resources.push_back(shaderResource);
//    }
//
//    // reflect specialization constants
//    std::vector<SpvReflectSpecializationConstant *> specializationConstants;
//    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(library.reflection, &count, nullptr));
//    specializationConstants.resize(count);
//    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(library.reflection, &count, specializationConstants.data()));
//
//    for (SpvReflectSpecializationConstant *constant : specializationConstants) {
//        ShaderResource shaderResource{};
//        shaderResource.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//        shaderResource.name = constant->name;
//        shaderResource.type = ShaderResourceType::SpecializationConstant;
//        shaderResource.constant_id = constant->constant_id;
//        resources.push_back(shaderResource);
//    }
//
//    return true;
//}

}