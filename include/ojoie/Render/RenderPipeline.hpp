//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_RENDERPIPELINE_HPP
#define OJOIE_RENDERPIPELINE_HPP

#include <ojoie/Math/Math.hpp>
#include <vector>

namespace AN {
struct RenderContext;
class Renderer;
}
namespace AN::RC {

enum class ShaderLibraryType {
    Vertex,
    Fragment,
    Geometry /// currently is not supported
};

class ShaderLibrary : private NonCopyable {
    struct Impl;
    Impl *impl;
    ShaderLibraryType _type;
    friend class RenderPipeline;
public:

    ShaderLibrary();

    ~ShaderLibrary();

    bool initWithSource(ShaderLibraryType type, const char *source);

    bool initWithPath(const struct AN::RenderContext &context, ShaderLibraryType type, const char *path);

    void deinit();

};

struct Function {
    const char *name;
    ShaderLibrary *library;
};

enum class VertexFormat {
    Float4,
    Float3,
    Float2
};

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

enum class BindingType {
    Uniform,
    Sampler,
    Texture,
    SamplerTexture
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
};

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
};
}

struct VertexDescriptor {
    typedef detail::HelperArray<VertexAttributeDescriptor, 7> VertexDescriptorArray;
    typedef detail::HelperArray<VertexLayoutDescriptor, 2> VertexLayoutArray;

    VertexDescriptorArray attributes; /// operator[attribute]
    VertexLayoutArray layouts;        /// operator[binding]
};


struct PushConstantDescriptor {
    uint32_t offset;
    uint32_t size;
    ShaderStageFlag stageFlag;
};

struct RenderPipelineDescriptor {
    typedef detail::HelperArray<RenderPipelineColorAttachmentDescriptor, 2> ColorAttachmentArray;
    typedef detail::HelperArray<BindingType, 4> BindingDescriptorArray;

    Function vertexFunction, fragmentFunction;
    VertexDescriptor vertexDescriptor;
    ColorAttachmentArray colorAttachments;
    DepthStencilDescriptor depthStencilDescriptor;
    BindingDescriptorArray bindings;
    uint32_t rasterSampleCount;
    bool alphaToCoverageEnabled;
    bool alphaToOneEnabled;

    bool pushConstantEnabled;
    PushConstantDescriptor pushConstantDescriptor;
};

/// \brief this class can only be use on render thread
class RenderPipeline : private NonCopyable {
    struct Impl;
    Impl *impl;


    void *getVkDescriptorLayout();

    void *getVkPipelineLayout();

    friend class AN::Renderer;
public:
    RenderPipeline();

    ~RenderPipeline();

    bool initWithPath(const char* vertexPath, const char* fragmentPath);

    bool initWithSource(const char *vertexSource, const char *fragmentSource);

    static RenderPipeline *Current();

    bool init(const RenderPipelineDescriptor &renderPipelineDescriptor);

    void deinit();

    void pushConstants(ShaderStageFlag stageFlag, uint32_t offset, uint32_t size, const void *data);

    void bind();

//    /// \brief activate after any binding
//    void activateBinding(const struct AN::RenderContext &context);

    /// \deprecated opengl specific utility uniform methods
    void setBool(const char *name, bool value) const;
    void setInt(const char *name, int value) const;
    void setFloat(const char name, float value) const;
    void setVec2(const char *name, const Math::vec2 &value) const;
    void setVec2(const char *name, float x, float y) const;
    // ------------------------------------------------------------------------
    void setVec3(const char *name, const Math::vec3 &value) const;
    void setVec3(const char *name, float x, float y, float z) const;
    // ------------------------------------------------------------------------
    void setVec4(const char *name, const Math::vec4 &value) const;
    void setVec4(const char *name, float x, float y, float z, float w) const;
    // ------------------------------------------------------------------------
    void setMat2(const char *name, const Math::mat2 &mat) const;
    // ------------------------------------------------------------------------
    void setMat3(const char *name, const Math::mat3 &mat) const;
    // ------------------------------------------------------------------------
    void setMat4(const char *name, const Math::mat4 &mat) const;
    
};



}

template<>
struct AN::enable_bitmask_operators<AN::RC::ColorWriteMask> : std::true_type {};

template<>
struct AN::enable_bitmask_operators<AN::RC::ShaderStageFlag> : std::true_type {};

#endif//OJOIE_RENDERPIPELINE_HPP
