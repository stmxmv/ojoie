#ifndef AN_COMMON_HLSL
#define AN_COMMON_HLSL


#define FLT_MIN  1.175494351e-38 // Minimum normalized positive floating-point number

// Normalize that account for vectors with zero length
float3 SafeNormalize(float3 inVec) {
    float dp3 = max(FLT_MIN, dot(inVec, inVec));
    return inVec * rsqrt(dp3);
}

#endif//AN_COMMON_HLSL
