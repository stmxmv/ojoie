#ifndef AN_SHADER_VARIABLES_FUNCTIONS_HLSL
#define AN_SHADER_VARIABLES_FUNCTIONS_HLSL

#include <SpaceTransforms.hlsl>

VertexPositionInputs GetVertexPositionInputs(float3 positionOS) {
    VertexPositionInputs input;
    input.positionWS = TransformObjectToWorld(positionOS);
    input.positionVS = TransformWorldToView(input.positionWS);
    input.positionCS = TransformWorldToHClip(input.positionWS);

    float4 ndc = input.positionCS * 0.5f;
    input.positionNDC.xy = float2(ndc.x, ndc.y * _ProjectionParams.x) + ndc.w;
    input.positionNDC.zw = input.positionCS.zw;

    return input;
}

VertexNormalInputs GetVertexNormalInputs(float3 normalOS) {
    VertexNormalInputs tbn;
    tbn.tangentWS = float3(1.0, 0.0, 0.0);
    tbn.bitangentWS = float3(0.0, 1.0, 0.0);
    tbn.normalWS = TransformObjectToWorldNormal(normalOS);
    return tbn;
}

VertexNormalInputs GetVertexNormalInputs(float3 normalOS, float4 tangentOS) {
    VertexNormalInputs tbn;

    // mikkts space compliant. only normalize when extracting normal at frag.
    float sign = tangentOS.w * GetOddNegativeScale();
    tbn.normalWS = TransformObjectToWorldNormal(normalOS);
    tbn.tangentWS = TransformObjectToWorldDir(tangentOS.xyz);
    tbn.bitangentWS = cross(tbn.normalWS, float3(tbn.tangentWS)) * sign;
    return tbn;
}

#endif//AN_SHADER_VARIABLES_FUNCTIONS_HLSL