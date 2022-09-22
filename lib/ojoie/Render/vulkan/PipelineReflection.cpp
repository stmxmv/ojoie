//
// Created by aojoie on 9/20/2022.
//

#include "Render/private/vulkan/PipelineReflection.hpp"

#include <SpvReflect/spirv_reflect.h>

namespace AN::VK {


bool PipelineReflection::reflectShaderResources(VkShaderStageFlagBits stage, const char *entryPoint,
                                                const void *spirv, uint64_t size,
                                                std::vector<ShaderResource> &resources) {

#define CHECK_RESULT(statement)                                                        \
    if (SpvReflectResult result = (statement); SPV_REFLECT_RESULT_SUCCESS != result) { \
        ANLog("%s return result code %d %s %d", #statement, result, __FILE__, __LINE__);                         \
        return false;                                                                  \
    }


    spv_reflect::ShaderModule shaderModule(size, spirv);

    /// reflect descriptor sets
    uint32_t count;
    std::vector<SpvReflectDescriptorSet *>  descriptorSets;
    CHECK_RESULT(shaderModule.EnumerateEntryPointDescriptorSets(entryPoint, &count, nullptr));
    descriptorSets.resize(count);
    CHECK_RESULT(shaderModule.EnumerateEntryPointDescriptorSets(entryPoint, &count, descriptorSets.data()));

    for (SpvReflectDescriptorSet *set : descriptorSets) {
        for (int i = 0; i < set->binding_count; ++i) {
            ShaderResource shaderResource{};

            SpvReflectDescriptorBinding *binding = set->bindings[i];

            shaderResource.stages = stage;
            shaderResource.set = set->set;
            shaderResource.binding = binding->binding;
            shaderResource.array_size = binding->count;
            shaderResource.name = binding->name;

            switch (binding->descriptor_type) {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    shaderResource.type = ShaderResourceType::Sampler;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    shaderResource.type = ShaderResourceType::ImageSampler;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    shaderResource.type = ShaderResourceType::Image;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    shaderResource.type = ShaderResourceType::ImageStorage;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    shaderResource.type = ShaderResourceType::BufferUniform;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    shaderResource.type = ShaderResourceType::BufferStorage;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    shaderResource.type = ShaderResourceType::BufferUniform;
                    shaderResource.dynamic = true;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    shaderResource.type = ShaderResourceType::BufferStorage;
                    shaderResource.dynamic = true;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    shaderResource.type = ShaderResourceType::InputAttachment;
                    break;
                default:
                    continue;
            }

            resources.push_back(shaderResource);
        }
    }

    /// reflect pushConstant
    std::vector<SpvReflectBlockVariable *> blocks;
    CHECK_RESULT(shaderModule.EnumerateEntryPointPushConstantBlocks(entryPoint, &count, nullptr));
    blocks.resize(count);
    CHECK_RESULT(shaderModule.EnumerateEntryPointPushConstantBlocks(entryPoint, &count, blocks.data()));

    for (SpvReflectBlockVariable *block : blocks) {
        ShaderResource shaderResource{};
        shaderResource.stages = stage;
        shaderResource.name = block->name;
        shaderResource.type = ShaderResourceType::PushConstant;
        shaderResource.offset = block->offset;
        shaderResource.size = block->size;
        resources.push_back(shaderResource);
    }

    // reflect specialization constants
    std::vector<SpvReflectSpecializationConstant *> specializationConstants;
    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(&shaderModule.GetShaderModule(), &count, nullptr));
    specializationConstants.resize(count);
    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(&shaderModule.GetShaderModule(), &count, specializationConstants.data()));

    for (SpvReflectSpecializationConstant *constant : specializationConstants) {
        ShaderResource shaderResource{};
        shaderResource.stages = stage;
        shaderResource.name = constant->name;
        shaderResource.type = ShaderResourceType::SpecializationConstant;
        shaderResource.constant_id = constant->constant_id;
        resources.push_back(shaderResource);
    }


    return true;
}


}