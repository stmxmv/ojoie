#ifndef AN_SPACE_TRANSFROMS_HLSL
#define AN_SPACE_TRANSFROMS_HLSL


float4x4 GetObjectToWorldMatrix() {
    return AN_MATRIX_M;
}

float4x4 GetWorldToObjectMatrix() {
    return AN_MATRIX_I_M;
}

float4x4 GetWorldToViewMatrix() {
    return AN_MATRIX_V;
}

// Transform to homogenous clip space
float4x4 GetWorldToHClipMatrix() {
    return AN_MATRIX_VP;
}

float GetOddNegativeScale() {
    return an_WorldTransformParams.w;
}

float3 TransformObjectToWorld(float3 positionOS) {
    return mul(GetObjectToWorldMatrix(), float4(positionOS, 1.0)).xyz;
}

float4 TransformObjectToHClip(float3 positionOS)
{
    // More efficient than computing M*VP matrix product
    return mul(GetWorldToHClipMatrix(), mul(GetObjectToWorldMatrix(), float4(positionOS, 1.0)));
}

float3 TransformWorldToView(float3 positionWS) {
    return mul(GetWorldToViewMatrix(), float4(positionWS, 1.0)).xyz;
}

float4 TransformWorldToHClip(float3 positionWS) {
    return mul(GetWorldToHClipMatrix(), float4(positionWS, 1.0));
}

// Transforms normal from object to world space
float3 TransformObjectToWorldNormal(float3 normalOS, bool doNormalize = true) {
#ifdef AN_ASSUME_UNIFORM_SCALING
    return TransformObjectToWorldDir(normalOS, doNormalize);
#else
    // Normal need to be multiply by inverse transpose
    float3 normalWS = mul(normalOS, (float3x3)GetWorldToObjectMatrix());
    if (doNormalize)
        return SafeNormalize(normalWS);

    return normalWS;
#endif
}

// Normalize to support uniform scaling
float3 TransformObjectToWorldDir(float3 dirOS, bool doNormalize = true) {
    float3 dirWS = mul((float3x3)GetObjectToWorldMatrix(), dirOS);
    if (doNormalize)
        return SafeNormalize(dirWS);

    return dirWS;
}


#endif//AN_SPACE_TRANSFROMS_HLSL