#ifndef AN_SHADOWS_HLSL
#define AN_SHADOWS_HLSL

#include "Common.hlsl"
#include "ShadowSamplingTent.hlsl"

#define BEYOND_SHADOW_FAR(shadowCoord) shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0

SamplerComparisonState sampler_LinearClampCompare;

Texture2D _MainLightShadowmapTexture;


CBUFFER_START(LightShadows)
    float4      _MainLightShadowParams; //  // (x: shadowStrength, y: >= 1.0 if soft shadows, 0.0 otherwise, z: main light fade scale, w: main light fade bias)
    float4      _MainLightShadowmapTexture_TexelSize;
    float4      _ShadowBias; // x: depth bias, y: normal bias
    float4x4    _MainLightWorldToShadow;
CBUFFER_END


float SampleShadowmapFiltered(TEXTURE2D_SHADOW_PARAM(ShadowMap, sampler_ShadowMap), float4 shadowCoord)
{
    float attenuation = float(1.0);

    float fetchesWeights[16];
    float2 fetchesUV[16];
    SampleShadow_ComputeSamples_Tent_7x7(_MainLightShadowmapTexture_TexelSize, shadowCoord.xy, fetchesWeights, fetchesUV);

    attenuation = fetchesWeights[0] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[0].xy, shadowCoord.z))
    + fetchesWeights[1] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[1].xy, shadowCoord.z))
    + fetchesWeights[2] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[2].xy, shadowCoord.z))
    + fetchesWeights[3] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[3].xy, shadowCoord.z))
    + fetchesWeights[4] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[4].xy, shadowCoord.z))
    + fetchesWeights[5] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[5].xy, shadowCoord.z))
    + fetchesWeights[6] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[6].xy, shadowCoord.z))
    + fetchesWeights[7] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[7].xy, shadowCoord.z))
    + fetchesWeights[8] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[8].xy, shadowCoord.z))
    + fetchesWeights[9] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[9].xy, shadowCoord.z))
    + fetchesWeights[10] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[10].xy, shadowCoord.z))
    + fetchesWeights[11] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[11].xy, shadowCoord.z))
    + fetchesWeights[12] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[12].xy, shadowCoord.z))
    + fetchesWeights[13] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[13].xy, shadowCoord.z))
    + fetchesWeights[14] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[14].xy, shadowCoord.z))
    + fetchesWeights[15] * SAMPLE_TEXTURE2D_SHADOW(ShadowMap, sampler_ShadowMap, float3(fetchesUV[15].xy, shadowCoord.z));
    

    return attenuation;
}

float SampleShadowmap(Texture2D ShadowMap, SamplerComparisonState sampler_ShadowMap, float4 shadowCoord)
{

    //shadowCoord.xyz /= shadowCoord.w;
    float attenuation;
    float shadowStrength = 1.f;

// #if (_SHADOWS_SOFT)
//     if(shadowParams.y > SOFT_SHADOW_QUALITY_OFF)
//     {
        attenuation = SampleShadowmapFiltered(TEXTURE2D_SHADOW_ARGS(ShadowMap, sampler_ShadowMap), shadowCoord);
//     }
//     else
// #endif
    // {
        // 1-tap hardware comparison
        // attenuation = float(ShadowMap.SampleCmpLevelZero(sampler_ShadowMap, shadowCoord.xy, shadowCoord.z));
    // }

    attenuation = LerpWhiteTo(attenuation, shadowStrength);

    // Shadow coords that fall out of the light frustum volume must always return attenuation 1.0
    // TODO: We could use branch here to save some perf on some platforms.
    return BEYOND_SHADOW_FAR(shadowCoord) ? 1.0 : attenuation;
}

half GetMainLightShadowFade(float3 positionWS)
{
    float3 camToPixel = positionWS - _WorldSpaceCameraPos;
    float distanceCamToPixel2 = dot(camToPixel, camToPixel);

    float fade = saturate(distanceCamToPixel2 * float(_MainLightShadowParams.z) + float(_MainLightShadowParams.w));
    return half(fade);
}

half MainLightRealtimeShadow(float4 shadowCoord)
{
    return SampleShadowmap(_MainLightShadowmapTexture, sampler_LinearClampCompare, shadowCoord);
}

half MainLightShadow(float4 shadowCoord, float3 positionWS)
{
    half realtimeShadow = MainLightRealtimeShadow(shadowCoord);
    half shadowFade = GetMainLightShadowFade(positionWS);
    return lerp(realtimeShadow, 1.0, shadowFade);
}

float4 TransformWorldToShadowCoord(float3 positionWS)
{
    float4 shadowCoord = mul(_MainLightWorldToShadow, float4(positionWS, 1.0));
    return float4(shadowCoord.xyz, 0);
}

float3 ApplyShadowBias(float3 positionWS, float3 normalWS, float3 lightDirection)
{
    float invNdotL = 1.0 - saturate(dot(lightDirection, normalWS));
    float scale = invNdotL * _ShadowBias.y;

    // normal bias is negative since we want to apply an inset normal offset
    positionWS = lightDirection * _ShadowBias.xxx + positionWS;
    positionWS = normalWS * scale.xxx + positionWS;
    return positionWS;
}

#endif//AN_SHADOWS_HLSL