//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_RENDERPIPELINESTATE_HPP
#define OJOIE_RENDERPIPELINESTATE_HPP

#include <ojoie/Math/Math.hpp>
#include <vector>
#include <map>

namespace AN {
struct RenderContext;
class Renderer;
}

#ifdef OJOIE_USE_VULKAN
namespace AN::VK {
class RenderCommandEncoder;
}
#endif
namespace AN::RC {


struct Function {
    const char *name;
    const char *library;
};

enum class VertexFormat {
    Float4,
    Float3,
    Float2,
    UChar4
};

/// \note apple metal have one more value as MTLVertexStepFunctionConstant
///       see https://developer.apple.com/documentation/metal/mtlvertexstepfunction/mtlvertexstepfunctionconstant
enum class VertexStepFunction {
    PerVertex,
    PerInstance,
};

enum class BlendOperation {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class ColorWriteMask {
    None  = 0,
    Red   = 1 << 0,
    Green = 1 << 1,
    Blue  = 1 << 2,
    Alpha = 1 << 3,
    All   = Red | Green | Blue | Alpha
};

enum class BlendFactor {
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationColor,
    OneMinusDestinationColor,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    SourceAlphaSaturated,
    BlendColor,
    OneMinusBlendColor,
    BlendAlpha,
    OneMinusBlendAlpha,
    Source1Color,
    OneMinusSource1Color,
    Source1Alpha,
    OneMinusSource1Alpha
};

enum class CompareFunction {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class ShaderStageFlag {
    None     = 0,
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Geometry = 1 << 2
};

struct StencilDescriptor {
    /// TODO
};

struct VertexAttributeDescriptor {
    VertexFormat format;
    uint32_t     offset;
    uint32_t    binding;
    uint32_t   location;
};

/// \note looks like vulkan doesn't have something like stepRate, as D3D12 has InstanceDataStepRate in
///       D3D12_INPUT_ELEMENT_DESC and apple metal has stepRate in MTLVertexBufferLayoutDescriptor
///       see https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc
///       , https://developer.apple.com/documentation/metal/mtlvertexbufferlayoutdescriptor?language=objc
///       and https://community.khronos.org/t/instancing-rate-in-vulkan/7286
struct VertexLayoutDescriptor {
    uint32_t stride;
    VertexStepFunction stepFunction;
};

struct RenderPipelineColorAttachmentDescriptor {
    bool blendingEnabled;
    BlendOperation alphaBlendOperation;
    BlendOperation rgbBlendOperation;
    ColorWriteMask writeMask;
    BlendFactor destinationAlphaBlendFactor;
    BlendFactor destinationRGBBlendFactor;
    BlendFactor sourceAlphaBlendFactor;
    BlendFactor sourceRGBBlendFactor;
};

struct DepthStencilDescriptor {
    bool depthTestEnabled;
    bool depthWriteEnabled;
    CompareFunction depthCompareFunction;
    StencilDescriptor backFaceStencil;
    StencilDescriptor frontFaceStencil;
};

namespace detail {
template<typename T, int default_capacity>
class HelperArray {
    mutable std::vector<T> array;

public:
    constexpr HelperArray() {
        array.reserve(default_capacity);
    }

    uint32_t count() const {
        return (uint32_t)array.size();
    }

    T &operator[](uint32_t index) const {
        if (index + 1 > array.size()) {
            array.resize(index + 1);
        }
        return array[index];
    }

    void reset() {
        array.clear();
    }
};
}

struct VertexDescriptor {
    typedef detail::HelperArray<VertexAttributeDescriptor, 7> VertexDescriptorArray;
    typedef detail::HelperArray<VertexLayoutDescriptor, 2> VertexLayoutArray;

    VertexDescriptorArray attributes; /// operator[attribute]
    VertexLayoutArray layouts;        /// operator[binding]
};


enum class CullMode {
    None  = 0,
    Front = 1 << 0,
    Back  = 1 << 1
};

/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState {
    // Map tracking state of the Specialization Constants
    std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state;
public:
    void reset() {
        specialization_constant_state.clear();
    }

    template<typename T>
    void setConstant(uint32_t constant_id, const T &data) {
        std::uint32_t value = static_cast<std::uint32_t>(data);

        setConstant(constant_id, std::to_address(data), sizeof(T));
    }

    void setConstant(uint32_t constant_id, const void *value, uint64_t size) {
        auto data = specialization_constant_state.find(constant_id);

        if (data != specialization_constant_state.end() && data->second.size() == size &&
            memcmp(data->second.data(), value, size) == 0) {
            return;
        }

        specialization_constant_state[constant_id].assign((uint8_t *)value, (uint8_t *)value + size);
    }

    const std::map<uint32_t, std::vector<uint8_t>> &getSpecializationConstantState() const {
        return specialization_constant_state;
    }
};

struct RenderPipelineStateDescriptor {
    typedef detail::HelperArray<RenderPipelineColorAttachmentDescriptor, 2> ColorAttachmentArray;

    Function vertexFunction, fragmentFunction;
    VertexDescriptor vertexDescriptor;
    ColorAttachmentArray colorAttachments;
    DepthStencilDescriptor depthStencilDescriptor;
    uint32_t rasterSampleCount;
    bool alphaToCoverageEnabled;
    bool alphaToOneEnabled;

    SpecializationConstantState specializationConstantState;

    CullMode cullMode;
    uint32_t subpassIndex;
};

class RenderPipelineState : private NonCopyable {
    void *impl{};

public:
    RenderPipelineState();

    RenderPipelineState(RenderPipelineState &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    ~RenderPipelineState();

    bool init(const RenderPipelineStateDescriptor &renderPipelineDescriptor);

    void deinit();

};



}

template<>
struct AN::enable_bitmask_operators<AN::RC::ColorWriteMask> : std::true_type {};

template<>
struct AN::enable_bitmask_operators<AN::RC::ShaderStageFlag> : std::true_type {};

#endif//OJOIE_RENDERPIPELINESTATE_HPP
