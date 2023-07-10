#ifndef AN_CORE_HLSL
#define AN_CORE_HLSL

#define CBUFFER_START(tag) cbuffer tag {
#define CBUFFER_END }

#define AN_DECLARE_TEX2D(tex) Texture2D tex; SamplerState sampler##tex
#define AN_DECLARE_TEX2D_NOSAMPLER(tex) Texture2D tex

#define AN_SAMPLE_TEX2D(tex,coord) tex.Sample (sampler##tex,coord)
#define AN_SAMPLE_TEX2D_SAMPLER(tex,samplertex,coord) tex.Sample (sampler##samplertex,coord)

#define TRANSFORM_TEX(tex, name) ((tex.xy) * name##_ST.xy + name##_ST.zw)

// Structs
struct VertexPositionInputs {
    float3 positionWS; // World space position
    float3 positionVS; // View space position
    float4 positionCS; // Homogeneous clip space position
    float4 positionNDC;// Homogeneous normalized device coordinates
};

struct VertexNormalInputs {
    float3 tangentWS;
    float3 bitangentWS;
    float3 normalWS;
};

#include <Common.hlsl>
#include <Input.hlsl>
#include <ShaderVariablesFunctions.hlsl>
#include "Packing.hlsl"

#endif//AN_CORE_HLSL

