// Unity built-in shader source. Copyright (c) 2016 Unity Technologies. MIT license (see license.txt)

Shader "Skybox/Procedural" {
Properties {
    _SunDisk ("Sun", Integer) = 2
    _SunSize ("Sun Size", Range(0,1)) = 0.04
    _SunSizeConvergence("Sun Size Convergence", Range(1,10)) = 5

    _AtmosphereThickness ("Atmosphere Thickness", Range(0,5)) = 1.0
    _SkyTint ("Sky Tint", Color) = (0.5, 0.5, 0.5, 1)
    _GroundColor ("Ground", Color) = (0.369, 0.349, 0.341, 1)

    _Exposure("Exposure", Range(0, 8)) = 1.3
}

SubShader {

    Tags { "Queue"="Background" "RenderType"="Background" "PreviewType"="Skybox" }



    Pass {

        Tags { "LightMode" = "Forward" }

        Cull Off 
        ZWrite Off

        HLSLPROGRAM
        #define vert vertex_main
        #define frag fragment_main

        #include "Core.hlsl"
        #include "Lighting.hlsl"

        CBUFFER_START(ANPerMaterial)
        float _Exposure;     // HDR exposure
        float4 _GroundColor;
        float _SunSize;
        float _SunSizeConvergence;
        float4 _SkyTint;
        float _AtmosphereThickness;
        CBUFFER_END

    #define AN_COLORSPACE_GAMMA

    #if defined(AN_COLORSPACE_GAMMA)
        #define GAMMA 2
        #define COLOR_2_GAMMA(color) color
        #define COLOR_2_LINEAR(color) color*color
        #define LINEAR_2_OUTPUT(color) sqrt(color)
    #else
        #define GAMMA 2.2
        // HACK: to get gfx-tests in Gamma mode to agree until UNITY_ACTIVE_COLORSPACE_IS_GAMMA is working properly
        #define COLOR_2_GAMMA(color) (pow(color,1.0 / GAMMA))
        #define COLOR_2_LINEAR(color) color
        #define LINEAR_2_LINEAR(color) color
    #endif

        // RGB wavelengths
        // .35 (.62=158), .43 (.68=174), .525 (.75=190)
        static const float3 kDefaultScatteringWavelength = float3(.65, .57, .475);
        static const float3 kVariableRangeForScatteringWavelength = float3(.15, .15, .15);

        #define OUTER_RADIUS 1.025
        static const float kOuterRadius = OUTER_RADIUS;
        static const float kOuterRadius2 = OUTER_RADIUS*OUTER_RADIUS;
        static const float kInnerRadius = 1.0;
        static const float kInnerRadius2 = 1.0;

        static const float kCameraHeight = 0.0001;

        #define kRAYLEIGH (lerp(0.0, 0.0025, pow(_AtmosphereThickness,2.5)))      // Rayleigh constant
        #define kMIE 0.0010             // Mie constant
        #define kSUN_BRIGHTNESS 20.0    // Sun brightness

        #define kMAX_SCATTER 50.0 // Maximum scattering value, to prevent math overflows on Adrenos

        static const float kHDSundiskIntensityFactor = 15.0;
        static const float kSimpleSundiskIntensityFactor = 27.0;

        static const float kSunScale = 400.0 * kSUN_BRIGHTNESS;
        static const float kKmESun = kMIE * kSUN_BRIGHTNESS;
        static const float kKm4PI = kMIE * 4.0 * 3.14159265;
        static const float kScale = 1.0 / (OUTER_RADIUS - 1.0);
        static const float kScaleDepth = 0.25;
        static const float kScaleOverScaleDepth = (1.0 / (OUTER_RADIUS - 1.0)) / 0.25;
        static const float kSamples = 2.0; // THIS IS UNROLLED MANUALLY, DON'T TOUCH

        #define MIE_G (-0.990)
        #define MIE_G2 0.9801

        #define SKY_GROUND_THRESHOLD 0.02

        // fine tuning of performance. You can override defines here if you want some specific setup
        // or keep as is and allow later code to set it according to target api

        // if set vprog will output color in final color space (instead of linear always)
        // in case of rendering in gamma mode that means that we will do lerps in gamma mode too, so there will be tiny difference around horizon
        // #define SKYBOX_COLOR_IN_TARGET_COLOR_SPACE 0

        // sun disk rendering:
        // no sun disk - the fastest option
        #define SKYBOX_SUNDISK_NONE 0
        // simplistic sun disk - without mie phase function
        #define SKYBOX_SUNDISK_SIMPLE 1
        // full calculation - uses mie phase function
        #define SKYBOX_SUNDISK_HQ 2

        // uncomment this line and change SKYBOX_SUNDISK_SIMPLE to override material settings
    #define SKYBOX_SUNDISK SKYBOX_SUNDISK_NONE

    #ifndef SKYBOX_SUNDISK
        #if defined(_SUNDISK_NONE)
            #define SKYBOX_SUNDISK SKYBOX_SUNDISK_NONE
        #elif defined(_SUNDISK_SIMPLE)
            #define SKYBOX_SUNDISK SKYBOX_SUNDISK_SIMPLE
        #else
            #define SKYBOX_SUNDISK SKYBOX_SUNDISK_HQ
        #endif
    #endif

    #ifndef SKYBOX_COLOR_IN_TARGET_COLOR_SPACE
        #if defined(SHADER_API_MOBILE)
            #define SKYBOX_COLOR_IN_TARGET_COLOR_SPACE 1
        #else
            #define SKYBOX_COLOR_IN_TARGET_COLOR_SPACE 0
        #endif
    #endif

        // Calculates the Rayleigh phase function
        half getRayleighPhase(half eyeCos2)
        {
            return 0.75 + 0.75*eyeCos2;
        }
        half getRayleighPhase(half3 light, half3 ray)
        {
            half eyeCos = dot(light, ray);
            return getRayleighPhase(eyeCos * eyeCos);
        }


        struct appdata_t
        {
            float3 vertex : POSITION;
        };

        struct v2f
        {
            float4  pos : SV_POSITION;
            half3   eyeRay : TEXCOORD1;
        };


        float scale(float inCos)
        {
            float x = 1.0 - inCos;
            return 0.25 * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
        }

        v2f vert (appdata_t v)
        {
            v2f OUT;
            OUT.pos = TransformObjectToHClip(v.vertex.xyz);

            // Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
            float3 eyeRay = normalize(mul((float3x3)an_ObjectToWorld, v.vertex.xyz));

            OUT.eyeRay = eyeRay;

            return OUT;
        }


        half4 frag (v2f IN) : SV_Target
        {

            float3 eyeRay = normalize(IN.eyeRay);
            half3 sky = lerp(half3(0.78, 0.96, 0.99), half3(0.28f, 0.38f, 0.59f), saturate(eyeRay.y * 0.5 + 0.05));
            half3 skyColor = _Exposure * COLOR_2_LINEAR(sky);
            half3 groundColor = _Exposure * COLOR_2_LINEAR(_GroundColor.rgb);

            float3 col = lerp(skyColor, groundColor, saturate(-eyeRay.y / SKY_GROUND_THRESHOLD));
            
             // Calculate the sun's color
            float3 sunColor = _MainLightColor.rgb;

            float sunIntensity = pow(saturate(dot(eyeRay, normalize(_MainLightPosition.xyz))), 2000.f);
            sunColor *= sunIntensity;

            col += sunColor;

            return half4(col,1.0);

        }
        ENDHLSL
    }
}

}