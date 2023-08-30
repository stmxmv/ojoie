//
// Created by aojoie on 4/10/2023.
//

#ifndef OJOIE_RENDERTYPES_HPP
#define OJOIE_RENDERTYPES_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Core/Name.hpp>
#include <ojoie/Template/Enumerate.hpp>
#include <ojoie/Template/SmallVector.hpp>
#include <ojoie/Serialize/SerializeDefines.h>
#include <ojoie/Math/Math.hpp>
#include <ranges>
#include <unordered_map>
#include <variant>

namespace AN {


inline static constexpr int kMaxFrameInFlight = 3;

enum PrimitiveType {
    kPrimitiveTriangles = 0,
    kPrimitiveQuads,
    kPrimitiveLines,
    kPrimitiveLineStrip,
    kPrimitivePoints,
    kPrimitiveTypeCount,
};

enum VertexChannelFormat {
    kChannelFormatFloat = 0,
    kChannelFormatFloat16,
    kChannelFormatColor,
    kChannelFormatByte,
    kChannelFormatCount
};

/// \note apple metal have one more value as MTLVertexStepFunctionConstant
///       see https://developer.apple.com/documentation/metal/mtlvertexstepfunction/mtlvertexstepfunctionconstant
enum VertexStepFunction {
    kVertexStepFunctionPerVertex,
    kVertexStepFunctionPerInstance,
};


enum ShaderChannel {
    kShaderChannelNone   = -1,
    kShaderChannelVertex = 0,// Vertex (vector3)
    kShaderChannelNormal,    // Normal (vector3)
    kShaderChannelColor,     // Vertex color
    kShaderChannelTexCoord0, // UV set 0 (vector2)
    kShaderChannelTexCoord1, // UV set 1 (vector2)
    kShaderChannelTangent,   // Tangent (vector4)
    kShaderChannelCount,     // Keep this last!
};

enum ShaderChannelMask {
    kShaderChannelsAll  = ((1 << kShaderChannelVertex) | (1 << kShaderChannelNormal) | (1 << kShaderChannelColor) | (1 << kShaderChannelTexCoord0) | (1 << kShaderChannelTexCoord1) | (1 << kShaderChannelTangent)),
    kShaderChannelsHot  = ((1 << kShaderChannelVertex) | (1 << kShaderChannelNormal) | (1 << kShaderChannelTangent)),
    kShaderChannelsCold = ((1 << kShaderChannelColor) | (1 << kShaderChannelTexCoord0) | (1 << kShaderChannelTexCoord1)),
};

#define VERTEX_FORMAT1(a) (1 << kShaderChannel##a)
#define VERTEX_FORMAT2(a,b) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b))
#define VERTEX_FORMAT3(a,b,c) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c))
#define VERTEX_FORMAT4(a,b,c,d) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c) | (1 << kShaderChannel##d))
#define VERTEX_FORMAT5(a,b,c,d,e) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c) | (1 << kShaderChannel##d) | (1 << kShaderChannel##e))
#define VERTEX_FORMAT6(a,b,c,d,e,f) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c) | (1 << kShaderChannel##d) | (1 << kShaderChannel##e) | (1 << kShaderChannel##f))

enum ShaderPropertyType {
    kShaderPropertyFloat = 0,
    kShaderPropertyMatrix,
    kShaderPropertyInt,
    kShaderPropertyBool,
    kShaderPropertyStruct,
    kShaderPropertyTexture,
    kShaderPropertySampler,
    kShaderPropertyTypeCount
};

enum ShaderResourceType {
    kShaderResourceImage = 0,
    kShaderResourceImageSampler,
    kShaderResourceImageStorage,
    kShaderResourceSampler,
    kShaderResourceBufferUniform,
    kShaderResourceBufferStorage,
    kShaderResourcePushConstant,
    kShaderResourceInputAttachment,
    kShaderResourceSpecializationConstant,
    kShaderResourceShaderResourceCount
};

inline static constexpr int kMaxVertexStreams = 4;

struct VertexAttributeDescriptor {
    VertexChannelFormat format;
    UInt32              dimension;
    UInt32              offset;
    UInt32              binding;
    UInt32              location;
};

/// \note looks like vulkan doesn't have something like stepRate, as D3D12 has InstanceDataStepRate in
///       D3D12_INPUT_ELEMENT_DESC and apple metal has stepRate in MTLVertexBufferLayoutDescriptor
///       see https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc
///       , https://developer.apple.com/documentation/metal/mtlvertexbufferlayoutdescriptor?language=objc
///       and https://community.khronos.org/t/instancing-rate-in-vulkan/7286
struct VertexLayoutDescriptor {
    UInt32             stride;
    VertexStepFunction stepFunction;
};

struct VertexDescriptor {
    SmallVector<VertexAttributeDescriptor, kShaderChannelCount> attributes;
    SmallVector<VertexLayoutDescriptor, kMaxVertexStreams>      layouts;
};

enum ShaderStage {
    kShaderStageVertex = 0,
    kShaderStageFragment,
    kShaderStageGeometry,
    kShaderStageCount
};


enum CompareFunction {
    kCompareFunctionNever = 0,
    kCompareFunctionLess,
    kCompareFunctionLEqual,
    kCompareFunctionEqual,
    kCompareFunctionGEqual,
    kCompareFunctionGreater,
    kCompareFunctionNotEqual,
    kCompareFunctionAlways,
    kCompareFunctionCount
};

enum BlendFactor {
    kBlendFactorZero = 0,
    kBlendFactorOne,
    kBlendFactorSourceColor,
    kBlendFactorOneMinusSourceColor,
    kBlendFactorSourceAlpha,
    kBlendFactorOneMinusSourceAlpha,
    kBlendFactorDestinationColor,
    kBlendFactorOneMinusDestinationColor,
    kBlendFactorDestinationAlpha,
    kBlendFactorOneMinusDestinationAlpha,
    kBlendFactorSourceAlphaSaturated,
    kBlendFactorBlendColor,
    kBlendFactorOneMinusBlendColor,
    kBlendFactorBlendAlpha,
    kBlendFactorOneMinusBlendAlpha,
    kBlendFactorSource1Color,
    kBlendFactorOneMinusSource1Color,
    kBlendFactorSource1Alpha,
    kBlendFactorOneMinusSource1Alpha,
    kBlendFactorCount
};

enum BlendOperation {
    kBlendOperationAdd = 0,
    kBlendOperationSubtract,
    kBlendOperationReverseSubtract,
    kBlendOperationMin,
    kBlendOperationMax,
    kBlendOperationCount
};

enum ColorWriteMaskBits {
    kColorWriteMaskNone  = 0,
    kColorWriteMaskRed   = 1 << 0,
    kColorWriteMaskGreen = 1 << 1,
    kColorWriteMaskBlue  = 1 << 2,
    kColorWriteMaskAlpha = 1 << 3,
    kColorWriteMaskAll   = kColorWriteMaskRed | kColorWriteMaskGreen | kColorWriteMaskBlue | kColorWriteMaskAlpha
};

typedef ANFlags ColorWriteMask;

enum CullMode {
    kCullModeNone = 0,
    kCullModeFront,
    kCullModeBack
};



enum RenderTargetFormat {
    kRTFormatDefault = 0,
    kRTFormatSwapchain,/// same as swapchain image format
    kRTFormatRGBA16Float,
    kRTFormatDepth,    // whatever is for "depth texture": Depth16 on GL, R32F on D3D9, ...
    kRTFormatShadowMap,// whatever is "native" (with built-in comparisons) shadow map format
    kRTFormatNormalMap,
    kRTFormatCount
};

enum PixelFormat {
    kPixelFormatR8Unorm = 0,
    kPixelFormatR8Unorm_sRGB,
    kPixelFormatRG8Unorm_sRGB,
    kPixelFormatRGBA8Unorm_sRGB,
    kPixelFormatRGBA8Unorm,
    kPixelFormatBGRA8Unorm_sRGB,
    kPixelFormatBGRA8Unorm,
    kPixelFormatRGBA16Float,

    kPixelFormatBC1_RGBA,
    kPixelFormatBC1_RGBA_sRGB,

    kPixelFormatBC2_RGBA,
    kPixelFormatBC2_RGBA_sRGB,

    kPixelFormatBC3_RGBA,
    kPixelFormatBC3_RGBA_sRGB,

    kPixelFormatBC4_RUnorm,
    kPixelFormatBC4_RSnorm,

    kPixelFormatBC5_RGUnorm,
    kPixelFormatBC5_RGSnorm,

    kPixelFormatBC6H_RGBUfloat,
    kPixelFormatBC6H_RGBFloat,

    kPixelFormatBC7_RGBAUnorm,
    kPixelFormatBC7_RGBAUnorm_sRGB,
    kPixelFormatCount
};


enum SamplerAddressMode {
    kSamplerAddressModeClampToEdge = 0,
    kSamplerAddressModeMirrorClampToEdge,
    kSamplerAddressModeRepeat,
    kSamplerAddressModeMirrorRepeat,
    kSamplerAddressModeClampToZero,
    kSamplerAddressModeClampToBorderColor,
    kSamplerAddressModeCount
};

enum SamplerBorderColor {
    kSamplerBorderColorTransparentBlack = 0,/// (0,0,0,0)
    kSamplerBorderColorOpaqueBlack,         /// (0,0,0,1)
    kSamplerBorderColorOpaqueWhite,         /// (1,1,1,1)
    kSamplerBorderColorCount
};

enum SamplerMinMagFilter {
    kSamplerMinMagFilterNearest = 0,
    kSamplerMinMagFilterLinear,
    kSamplerMinMagFilterCount
};

enum SamplerMipFilter {
    kSamplerMipFilterNotMipmapped = 0,
    kSamplerMipFilterNearest,
    kSamplerMipFilterLinear,
    kSamplerMipFilterCount
};

enum SamplerFilterMode {
    kSamplerFilterNearest = 0,
    kSamplerFilterBilinear,
    kSamplerFilterTrilinear,
    kSamplerFilterCount
};

struct SamplerDescriptor {
    bool                normalizedCoordinates;
    SamplerAddressMode  addressModeU;
    SamplerAddressMode  addressModeV;
    SamplerAddressMode  addressModeW;
    SamplerBorderColor  borderColor;
    SamplerFilterMode   filter;
    uint32_t            maxAnisotropy;
    CompareFunction     compareFunction;
    float               lodMinClamp, lodMaxClamp;

    DECLARE_SERIALIZE_NO_IDPTR(SamplerDescriptor)
};

template<typename Coder>
void SamplerDescriptor::transfer(Coder &coder) {
    TRANSFER(normalizedCoordinates);
    TRANSFER(addressModeU);
    TRANSFER(addressModeV);
    TRANSFER(addressModeW);
    TRANSFER(borderColor);
    TRANSFER(filter);
    TRANSFER(maxAnisotropy);
    TRANSFER(compareFunction);
    TRANSFER(lodMinClamp);
    TRANSFER(lodMaxClamp);
}

struct ShaderVertexInput {
    UInt8               location;
    VertexChannelFormat format;
    UInt8               dimension;
    std::string         semantic;

    DECLARE_SERIALIZE_NO_IDPTR(ShaderVertexInput)
};

template<typename Coder>
void ShaderVertexInput::transfer(Coder &coder) {
    TRANSFER(location);
    coder.transfer((std::underlying_type_t<VertexChannelFormat> &)format, "format");
    TRANSFER(dimension);
    TRANSFER(semantic);
}

struct ShaderProperty {
    int                set, binding;
    ShaderResourceType resourceType;
    ShaderPropertyType propertyType;
    ShaderStage   stage;
    int                dimension;

    int constant_id;// if resourceType is kShaderResourceSpecializationConstant

    // if resourceType is buffer
    int offset;/// absolute offset in the binding
    int size;

    Name name;

    DECLARE_SERIALIZE_NO_IDPTR(ShaderProperty)
};

template<typename Coder>
void ShaderProperty::transfer(Coder &coder) {
    TRANSFER(set);
    TRANSFER(binding);
    coder.transfer((std::underlying_type_t<ShaderResourceType> &)resourceType, "resourceType");
    coder.transfer((std::underlying_type_t<ShaderPropertyType> &)propertyType, "propertyType");
    TRANSFER(stage);
    TRANSFER(dimension);
    TRANSFER(constant_id);
    TRANSFER(offset);
    TRANSFER(size);
    TRANSFER(name);
}

struct ShaderResource {

    struct Block {
        ShaderPropertyType type;
        int                offset;
        int                absolute_offset;
        int                size;
        int                dimension;
        std::string        name;
        std::vector<Block> members;
    };

    ShaderStage   stage;
    ShaderResourceType resourceType;
    ShaderPropertyType propertyType;
    uint32_t           set;
    uint32_t           binding;
    uint32_t           array_size;

    uint32_t input_attachment_index;

    uint32_t constant_id;
    union {
        UInt32 int_bool_value;
        float  float_value;
    } default_value;

    bool        dynamic;
    std::string name;

    Block block;
};

typedef std::vector<ShaderProperty> ShaderPropertyList;


struct StencilDescriptor {
    /// TODO
};

struct RenderPipelineColorAttachmentDescriptor {
    bool           blendingEnabled;
    BlendOperation alphaBlendOperation;
    BlendOperation rgbBlendOperation;
    ColorWriteMask writeMask;
    BlendFactor    destinationAlphaBlendFactor;
    BlendFactor    destinationRGBBlendFactor;
    BlendFactor    sourceAlphaBlendFactor;
    BlendFactor    sourceRGBBlendFactor;
};

struct DepthStencilDescriptor {
    bool              depthTestEnabled;
    bool              depthWriteEnabled;
    CompareFunction   depthCompareFunction;
    StencilDescriptor backFaceStencil;
    StencilDescriptor frontFaceStencil;
};



/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState {
    // Map tracking state of the Specialization Constants
    SmallVector<std::pair<UInt32, UInt32>> vec;

public:
    void reset() {
        vec.clear();
    }

    template<typename T>
        requires(sizeof(T) == sizeof(UInt32))
    void setConstant(UInt32 constant_id, T data) {
        UInt32 value = *reinterpret_cast<UInt32 *>(&data);
        vec.emplace_back(constant_id, value);
    }

    std::span<const std::pair<UInt32, UInt32>> getSpecializationMap() const {
        return vec;
    }
};


struct ShaderFunctionDescriptor {
    const UInt8 *code;
    size_t       size;
    const char  *entry;
};

struct RenderPipelineStateDescriptor {

    ShaderFunctionDescriptor vertexFunction, fragmentFunction;

    SmallVector<RenderPipelineColorAttachmentDescriptor, 2> colorAttachments;

    DepthStencilDescriptor depthStencilDescriptor;
    uint32_t               rasterSampleCount;
    bool                   alphaToCoverageEnabled;
    bool                   alphaToOneEnabled;

    SpecializationConstantState specializationConstantState;

    CullMode cullMode;
};

enum AttachmentLoadOp {
    kAttachmentLoadOpLoad = 0,
    kAttachmentLoadOpClear,
    kAttachmentLoadOpDontCare,
    kAttachmentLoadOpCount
};

enum AttachmentStoreOp {
    kAttachmentStoreOpStore = 0,
    kAttachmentStoreOpDontCare,
    kAttachmentStoreOpNone,
    kAttachmentStoreOpCount
};

struct LoadStoreInfo {
    AttachmentLoadOp  loadOp;
    AttachmentStoreOp storeOp;
};

struct SubpassInfo {
    /// below are the indices of renderTarget attachments array
    SmallVector<UInt32>  inputAttachments;
    SmallVector<UInt32>  colorAttachments;
    std::optional<Int32> depthStencilAttachment;
    std::optional<Int32> resolveAttachment;
};

typedef union ClearColorValue {
    float  float32[4];
    UInt32 int32[4];
    Int32  uint32[4];
} ClearColorValue;

typedef struct ClearDepthStencilValue {
    float  depth;
    UInt32 stencil;
} ClearDepthStencilValue;

typedef union ClearValue {
    ClearColorValue        color;
    ClearDepthStencilValue depthStencil;
} ClearValue;

class RenderTarget;

typedef UInt64 TextureID;

struct AttachmentDescriptor {
    RenderTargetFormat format;
    Vector4f clearColor = { 0.f, 0.f, 0.f, 1.f };
    float clearDepth = 1.f;
    float clearStencil = 0.f;
    AttachmentLoadOp  loadOp;
    AttachmentStoreOp storeOp;
    RenderTarget *loadStoreTarget;
    RenderTarget *resolveTarget;
};

struct RenderTargetDescriptor {
    RenderTargetFormat format;
    UInt32             width;
    UInt32             height;
    UInt32             samples;
};

struct RenderPassDescriptor {
    SmallVector<AttachmentDescriptor> attachments;
    SmallVector<LoadStoreInfo>        loadStoreInfos;
    SmallVector<SubpassInfo>          subpasses;
};

/// some anti-aliasing can combine with others
enum AntiAliasingMethodBits {
    kAntiAliasingNone = 0,
    kAntiAliasingMSAA = 1,
    kAntiAliasingTAA  = 1 << 2,
    kAntiAliasingFSR  = 1 << 3,
    AntiAliasingFXAA  = 1 << 4
};

typedef ANFlags AntiAliasingMethod;

struct Viewport {
    float originX, originY;
    float width, height;
    float znear = 0.f, zfar = 1.f;
};

struct ScissorRect {
    int32_t x, y;
    int32_t width, height;
};


}// namespace AN

#endif//OJOIE_RENDERTYPES_HPP
