//
// Created by aojoie on 9/20/2022.
//

#include <ojoie/Utility/Log.h>
#include "Render/PipelineReflection.hpp"

#ifdef OJOIE_USE_VULKAN
#include <SpvReflect/spirv_reflect.h>
#endif

#include <ojoie/Render/private/D3D11/Common.hpp>
#include <D3DCompiler.h>

namespace AN {

#ifdef OJOIE_USE_VULKAN
ShaderStageFlags to_ANRenderType(SpvReflectShaderStageFlagBits stage) {
    ShaderStageFlags ret = 0;
    if (stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
        ret |= kShaderStageVertex;
    }
    if (stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT) {
        ret |= kShaderStageFragment;
    }
    if (stage & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT) {
        ret |= kShaderStageGeometry;
    }
    return ret;
}

static bool collect_member_recur(const SpvReflectBlockVariable &block, ShaderResource::Block &outBlock) {
    outBlock.name                               = block.name;
    outBlock.size                               = (int) block.size;
    outBlock.offset                             = (int) block.offset;
    outBlock.absolute_offset                    = (int) block.absolute_offset;
    SpvReflectTypeDescription *type_description = block.type_description;
    if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
        outBlock.type      = kShaderPropertyMatrix;
        outBlock.dimension = (int) type_description->traits.numeric.matrix.column_count;
        if (type_description->traits.numeric.matrix.column_count != type_description->traits.numeric.matrix.row_count) {
            AN_LOG(Error, "not support matrix type with different row and col count");
            return false;
        }

    } else if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
        outBlock.type      = kShaderPropertyFloat;
        outBlock.dimension = (int) type_description->traits.numeric.vector.component_count;

    } else if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        outBlock.type      = kShaderPropertyFloat;
        outBlock.dimension = 1;

    } else if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
        outBlock.type      = kShaderPropertyBool;
        outBlock.dimension = 1;
    } else if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
        outBlock.type      = kShaderPropertyInt;
        outBlock.dimension = 1;
    } else if (type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
        outBlock.type      = kShaderPropertyStruct;
        outBlock.dimension = 1;
    } else {
        AN_LOG(Error, "Unsupported member type %d", block.type_description->type_flags);
        return false;
    }

    if (block.member_count > 0) {
        for (int i = 0; i < block.member_count; ++i) {
            ShaderResource::Block member;
            if (!collect_member_recur(block.members[i], member)) {
                return false;
            }
            outBlock.members.push_back(member);
        }
    }
    return true;
}
#endif //OJOIE_USE_VULKAN
static bool collect_member_recur(ID3D11ShaderReflectionType *type, ShaderResource::Block &outBlock, int parentOffset = 0) {
    D3D11_SHADER_TYPE_DESC desc;
    HRESULT hr;
    hr = type->GetDesc(&desc);
    ANAssert(SUCCEEDED(hr));
    outBlock.name                               = desc.Name;
//    outBlock.size                               = ;
    outBlock.offset                             = (int) desc.Offset;
    outBlock.absolute_offset                    = (int) parentOffset + desc.Offset;

    if (desc.Class == D3D_SVC_MATRIX_ROWS || desc.Class == D3D_SVC_MATRIX_COLUMNS) {
        outBlock.type      = kShaderPropertyMatrix;
        outBlock.dimension = (int) desc.Rows;
        if (desc.Rows != desc.Columns) {
            AN_LOG(Error, "not support matrix type with different row and col count");
            return false;
        }
    } else if (desc.Class == D3D_SVC_VECTOR) {

        ANAssert(desc.Type == D3D_SVT_FLOAT);
        outBlock.type      = kShaderPropertyFloat;
        outBlock.dimension = (int) desc.Columns;

    } else if (desc.Class == D3D_SVC_SCALAR ) {

        if (desc.Type == D3D_SVT_FLOAT) {
            outBlock.type      = kShaderPropertyFloat;
            outBlock.dimension = 1;
        } else if (desc.Type == D3D_SVT_INT || desc.Type == D3D_SVT_UINT) {
            outBlock.type      = kShaderPropertyInt;
            outBlock.dimension = 1;

        } else if (desc.Type == D3D_SVT_BOOL) {
            outBlock.type      = kShaderPropertyBool;
            outBlock.dimension = 1;
        }

    } else if (desc.Class == D3D_SVC_STRUCT) {
        outBlock.type      = kShaderPropertyStruct;
        outBlock.dimension = 1;
    } else {
        AN_LOG(Error, "Unsupported member type %d", desc.Class);
        return false;
    }

    if (desc.Class == D3D_SVC_STRUCT && desc.Members > 0) {
        for (int i = 0; i < desc.Members; ++i) {

            ID3D11ShaderReflectionType *subType = type->GetMemberTypeByIndex(i);

            ShaderResource::Block member;
            if (!collect_member_recur(subType, member, parentOffset + desc.Offset)) {
                return false;
            }
            outBlock.members.push_back(member);
        }
    }
    return true;
}

static void collect_resource_buffer_property_recur(int set, int binding,
                                                   ShaderResourceType           type,
                                                   ShaderStage             stage,
                                                   const ShaderResource::Block &block,
                                                   ShaderPropertyList           &outList) {

    Name            name(block.name);
    ShaderProperty property;
    property.resourceType    = type;
    property.propertyType    = block.type;
    property.stage           = stage;
    property.name            = name;
    property.set             = set;
    property.binding         = binding;
    property.offset          = block.absolute_offset;/// property use absolute offset
    property.size            = block.size;
    property.dimension       = block.dimension;

    outList.push_back(property);

    for (const auto &member : block.members) {
        collect_resource_buffer_property_recur(set, binding, type, stage, member, outList);
    }
}

///// add resource if name not exist, if exist it will add shader stages
//__force_inline void addResourceIfNameNotExist(ShaderResource &&shaderResource, std::vector<ShaderResource> &resources) {
//    if (auto it = std::find_if(resources.begin(), resources.end(), [&](auto &&res) {
//            return res.name == shaderResource.name;
//        });
//        it != resources.end()) {
//
//        it->stages |= shaderResource.stages;
//    } else {
//        resources.push_back(shaderResource);
//    }
//}

bool PipelineReflection::reflectSPIRV(const void *spirv, uint64_t size) {

#ifdef OJOIE_USE_VULKAN

#define CHECK_RESULT(statement)                                                                    \
    if (SpvReflectResult result = (statement); SPV_REFLECT_RESULT_SUCCESS != result) {             \
        AN_LOG(Warning, "%s return result code %d %s %d", #statement, result, __FILE__, __LINE__); \
        return false;                                                                              \
    }


    spv_reflect::ShaderModule shaderModule(size, spirv, SPV_REFLECT_MODULE_FLAG_NO_COPY);
    const char               *entryPoint = shaderModule.GetEntryPointName();

    /// check some requirements
    if (shaderModule.GetEntryPointCount() > 1) {
        AN_LOG(Error, "Current not support multi spv entry point");
        return false;
    }

    SpvReflectShaderStageFlagBits vkStage = shaderModule.GetShaderStage();
    ShaderStageFlags      stage   = to_ANRenderType(vkStage);

    /// reflect vertex input
    uint32_t count;
    if (stage == kShaderStageVertex) {
        std::vector<SpvReflectInterfaceVariable *> inputVariables;
        CHECK_RESULT(shaderModule.EnumerateEntryPointInputVariables(entryPoint, &count, nullptr));
        inputVariables.resize(count);
        CHECK_RESULT(shaderModule.EnumerateEntryPointInputVariables(entryPoint, &count, inputVariables.data()));

        for (SpvReflectInterfaceVariable *var : inputVariables) {
            if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                /// skip built in
                continue;
            }
            ShaderVertexInput input;
            input.location = var->location;
            input.semantic = var->semantic;
            switch (var->format) {
                case SPV_REFLECT_FORMAT_R32_SFLOAT:
                    input.format    = kChannelFormatFloat;
                    input.dimension = 1;
                    break;
                case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
                    input.format    = kChannelFormatFloat;
                    input.dimension = 2;
                    break;
                case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
                    input.format    = kChannelFormatFloat;
                    input.dimension = 3;
                    break;
                case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
                    input.format    = kChannelFormatFloat;
                    input.dimension = 4;
                    break;
                default:
                    AN_LOG(Warning, "Not support spv input variable format %d", var->format);
                    return false;
            }

            /// special case COLOR0
            if (input.semantic == "COLOR0") {
                input.format    = kChannelFormatColor;
                input.dimension = 4;
            }

            _vertexInputs.push_back(input);
        }
    }

    /// reflect descriptor sets

    std::vector<SpvReflectDescriptorSet *> descriptorSets;
    CHECK_RESULT(shaderModule.EnumerateEntryPointDescriptorSets(entryPoint, &count, nullptr));
    descriptorSets.resize(count);
    CHECK_RESULT(shaderModule.EnumerateEntryPointDescriptorSets(entryPoint, &count, descriptorSets.data()));

    for (SpvReflectDescriptorSet *set : descriptorSets) {
        for (int i = 0; i < set->binding_count; ++i) {
            ShaderResource shaderResource{};

            SpvReflectDescriptorBinding *binding = set->bindings[i];

            shaderResource.stages     = stage;
            shaderResource.set        = set->set;
            shaderResource.binding    = binding->binding;
            shaderResource.array_size = binding->count;
            shaderResource.name       = binding->name;

            switch (binding->descriptor_type) {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    shaderResource.resourceType = kShaderResourceSampler;
                    shaderResource.propertyType = kShaderPropertySampler;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    shaderResource.resourceType = kShaderResourceImageSampler;
                    shaderResource.propertyType = kShaderPropertyTexture;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    shaderResource.resourceType = kShaderResourceImage;
                    shaderResource.propertyType = kShaderPropertyTexture;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    shaderResource.resourceType = kShaderResourceImageStorage;
                    shaderResource.propertyType = kShaderPropertyTexture;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    shaderResource.resourceType = kShaderResourceBufferUniform;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    shaderResource.resourceType = kShaderResourceBufferStorage;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    /// dynamic here is just a hint, dynamic is controlled by the pipeline
                    shaderResource.resourceType = kShaderResourceBufferUniform;
                    shaderResource.dynamic      = true;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    shaderResource.resourceType = kShaderResourceBufferStorage;
                    shaderResource.dynamic      = true;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    shaderResource.resourceType           = kShaderResourceInputAttachment;
                    shaderResource.propertyType           = kShaderPropertyTexture;
                    shaderResource.input_attachment_index = binding->input_attachment_index;
                    break;
                default:
                    continue;
            }

            if (binding->type_description->decoration_flags & (SPV_REFLECT_DECORATION_BLOCK | SPV_REFLECT_DECORATION_BUFFER_BLOCK)) {
                if (!collect_member_recur(binding->block, shaderResource.block)) {
                    return false;
                }
                shaderResource.propertyType = shaderResource.block.type;
            }

            addResourceIfNameNotExist(std::move(shaderResource), _resources);
        }
    }

    /// reflect pushConstant
    std::vector<SpvReflectBlockVariable *> blocks;
    CHECK_RESULT(shaderModule.EnumerateEntryPointPushConstantBlocks(entryPoint, &count, nullptr));
    blocks.resize(count);
    CHECK_RESULT(shaderModule.EnumerateEntryPointPushConstantBlocks(entryPoint, &count, blocks.data()));

    for (SpvReflectBlockVariable *block : blocks) {
        ShaderResource shaderResource{};
        shaderResource.stages       = stage;
        shaderResource.name         = block->name;
        shaderResource.resourceType = kShaderResourcePushConstant;

        if (!collect_member_recur(*block, shaderResource.block)) {
            return false;
        }

        shaderResource.propertyType = shaderResource.block.type;

        addResourceIfNameNotExist(std::move(shaderResource), _resources);
    }

    // reflect specialization constants
    std::vector<SpvReflectSpecializationConstant *> specializationConstants;
    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(&shaderModule.GetShaderModule(), &count, nullptr));
    specializationConstants.resize(count);
    CHECK_RESULT(spvReflectEnumerateSpecializationConstants(&shaderModule.GetShaderModule(), &count, specializationConstants.data()));

    for (SpvReflectSpecializationConstant *constant : specializationConstants) {
        ShaderResource shaderResource{};
        shaderResource.stages       = stage;
        shaderResource.name         = constant->name;
        shaderResource.resourceType = kShaderResourceSpecializationConstant;
        shaderResource.constant_id  = constant->constant_id;

        switch (constant->constant_type) {
            case SPV_REFLECT_SPECIALIZATION_CONSTANT_BOOL:
                shaderResource.propertyType                 = kShaderPropertyBool;
                shaderResource.default_value.int_bool_value = constant->default_value.int_bool_value;
                break;
            case SPV_REFLECT_SPECIALIZATION_CONSTANT_INT:
                shaderResource.propertyType                 = kShaderPropertyInt;
                shaderResource.default_value.int_bool_value = constant->default_value.int_bool_value;
                break;
            case SPV_REFLECT_SPECIALIZATION_CONSTANT_FLOAT:
                shaderResource.propertyType              = kShaderPropertyFloat;
                shaderResource.default_value.float_value = constant->default_value.float_value;
                break;
        }

        addResourceIfNameNotExist(std::move(shaderResource), _resources);
    }

    /// sort resources by set and binding
    std::ranges::sort(_resources, [](auto &&a, auto &&b) {
        return std::tie(a.set, a.binding) < std::tie(b.set, b.binding);
    });

    return true;
#else
    return false;
#endif //OJOIE_USE_VULKAN
}


bool PipelineReflection::reflectCSO(const void *cso, uint64_t size, ShaderStage stage) {

    D3D11::ComPtr<ID3D11ShaderReflection> reflection;
    HRESULT hr = D3DReflect(cso, size, __uuidof(ID3D11ShaderReflection), &reflection);

    if (FAILED(hr)) {
        AN_LOG(Error, "Fail to reflect d3d11 shader");
        return false;
    }
    D3D11_SHADER_DESC desc;

    hr = reflection->GetDesc(&desc);

    ANAssert(SUCCEEDED(hr));

    if (stage == kShaderStageVertex) {
        for (int i = 0; i < desc.InputParameters; ++i) {
            D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
            hr = reflection->GetInputParameterDesc(i, &paramDesc);
            ANAssert(SUCCEEDED(hr));

            if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) {
                continue; // skip system type
            }

            ShaderVertexInput input;
            input.location = paramDesc.Register;
            input.semantic = paramDesc.SemanticName;

            switch (paramDesc.ComponentType) {
                case D3D_REGISTER_COMPONENT_FLOAT32:
                    input.format = kChannelFormatFloat;
                    break;
                default:
                case D3D_REGISTER_COMPONENT_UNKNOWN:
                    AN_LOG(Error, "Unknown input parameter type");
                    break;
            }

            switch (paramDesc.Mask) {
                case 0b00000001:
                    input.dimension = 1;
                    break;
                case 0b00000011:
                    input.dimension = 2;
                    break;
                case 0b00000111:
                    input.dimension = 3;
                    break;
                case 0b00001111:
                    input.dimension = 4;
                    break;
                default:
                    input.dimension = 4;
                    break;
            }

            /// special case COLOR0
            if (input.semantic == "COLOR0") {
                input.format    = kChannelFormatColor;
                input.dimension = 4;
            }

            _vertexInputs.push_back(input);
        }
    }


    for (int i = 0; i < desc.BoundResources; ++i) {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        reflection->GetResourceBindingDesc(i, &bindDesc);

        ShaderResource shaderResource{};

        shaderResource.stage     = stage;
        shaderResource.set        = 0;
        shaderResource.binding    = bindDesc.BindPoint;
        shaderResource.array_size = bindDesc.BindCount;
        shaderResource.name       = bindDesc.Name;

        switch (bindDesc.Type) {
            case D3D_SIT_TEXTURE:
                shaderResource.resourceType = kShaderResourceImage;
                shaderResource.propertyType = kShaderPropertyTexture;
                break;
            case D3D_SIT_SAMPLER:
                shaderResource.resourceType = kShaderResourceSampler;
                shaderResource.propertyType = kShaderPropertySampler;
                break;
            case D3D_SIT_CBUFFER:
                shaderResource.resourceType = kShaderResourceBufferUniform;
                shaderResource.propertyType = kShaderPropertyStruct;
                break;

            default:
                AN_LOG(Error, "Unknown shader resource type");
                break;
        }

        if (bindDesc.Type == D3D_SIT_CBUFFER) {
            ID3D11ShaderReflectionConstantBuffer *cb;
            cb = reflection->GetConstantBufferByName(bindDesc.Name);
            D3D11_SHADER_BUFFER_DESC bufferDesc;
            hr = cb->GetDesc(&bufferDesc);
            ANAssert(SUCCEEDED(hr));

            shaderResource.block.type = kShaderPropertyStruct;
            shaderResource.block.name = shaderResource.name;
            shaderResource.block.size = bufferDesc.Size;

            for (int j = 0; j < bufferDesc.Variables; ++j) {
                ID3D11ShaderReflectionVariable *variable = cb->GetVariableByIndex(j);
                D3D11_SHADER_VARIABLE_DESC varDesc;
                hr = variable->GetDesc(&varDesc);
                ANAssert(SUCCEEDED(hr));

                shaderResource.block.members.emplace_back();
                if (!collect_member_recur(variable->GetType(),
                                          shaderResource.block.members.back())) {
                    return false;
                }

                shaderResource.block.members.back().name = varDesc.Name;
                shaderResource.block.members.back().offset = varDesc.StartOffset;
                shaderResource.block.members.back().absolute_offset = varDesc.StartOffset;
            }
        }
        _resources.push_back(shaderResource);
//        addResourceIfNameNotExist(std::move(shaderResource), _resources);
    }

    return true;
}

const ShaderVertexInput *PipelineReflection::getVertexInput(const char *semantic) const {
    for (const auto &i : _vertexInputs) {
        if (i.semantic == semantic) {
            return &i;
        }
    }
    return nullptr;
}

const ShaderResource *PipelineReflection::getResource(const char *name, ShaderStage stage) const {
    for (const auto &res : _resources) {
        if (res.name == name && res.stage == stage) {
            return &res;
        }
    }
    return nullptr;
}

ShaderPropertyList PipelineReflection::buildPropertyList() const {
    ShaderPropertyList ret;
    for (const auto &res : _resources) {

        if (res.propertyType == kShaderPropertyStruct) {

            collect_resource_buffer_property_recur((int) res.set, (int) res.binding,
                                                   res.resourceType,
                                                   res.stage,
                                                   res.block,
                                                   ret);

        } else {

            /// non block type
            Name            name(res.name);
            ShaderProperty property;
            property.resourceType    = res.resourceType;
            property.propertyType    = res.propertyType;
            property.stage           = res.stage;
            property.name            = name;
            property.set             = (int) res.set;
            property.binding         = (int) res.binding;
            property.constant_id     = (int) res.constant_id;
            ret.push_back(property);
        }
    }
    return ret;
}

}// namespace AN