#ifndef AN_REALTIMELIGHTS_HLSL
#define AN_REALTIMELIGHTS_HLSL

// Abstraction over Light shading data.
struct Light {
    half3   direction;
    half3   color;
    float   distanceAttenuation; // full-float precision required on some platforms
    half    shadowAttenuation;
    uint    layerMask;
};


Light GetMainLight() {
    Light light;
    light.direction = half3(_MainLightPosition.xyz);
#if USE_CLUSTERED_LIGHTING
    light.distanceAttenuation = 1.0;
#else
    light.distanceAttenuation = an_LightData.z; // unity_LightData.z is 1 when not culled by the culling mask, otherwise 0.
#endif
    light.shadowAttenuation = 1.0;
    light.color = _MainLightColor.rgb;

#ifdef _LIGHT_LAYERS
    light.layerMask = _MainLightLayerMask;
#else
    light.layerMask = DEFAULT_LIGHT_LAYERS;
#endif

    return light;
}


#endif//AN_REALTIMELIGHTS_HLSL