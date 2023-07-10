#ifndef AN_PACKING_HLSL
#define AN_PACKING_HLSL

float3 UnpackNormalRGBNoScale(float4 packedNormal) 
{
    return packedNormal.rgb * 2.0 - 1.0;
}

#endif//AN_PACKING_HLSL