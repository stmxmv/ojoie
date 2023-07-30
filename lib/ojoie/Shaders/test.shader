Shader "AN/DiffuseVertexLevel"
{
    Properties
    {
        _DiffuseTex ("Diffuse Tex", 2D) = "white" { }
        _Specular ("Specular", Color) = (1.0, 1.0, 1.0, 1.0)
        _NormalMapToggle ("Use Normal Map", Float) = 0
        _NormalMap ("Normal Map", 2D) = "bump" { }
        _NormalScale ("Normal Scale", Float) = 1.0
        _Gloss ("Gloss", Range(1, 256)) = 20
        _ADD_LIGHT ("AdditionalLights", Float) = 0
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"
        #include "Lighting.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_DiffuseTex);
            AN_DECLARE_TEX2D_NOSAMPLER(_NormalMap);
            // float4 _DiffuseTex_ST;
//            float4 _Specular;
//            float _Gloss;
        CBUFFER_END

        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            Cull Back
            Blend SrcColor OneMinusSrcAlpha
            BlendOp Add
            ColorMask RGB
            ZTest LEqual
            ZWrite On
            ZClip True

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry

            #define _NORMALMAP

            struct appdata
            {
                float4 vertex : POSITION;
                half3 normal : NORMAL;
                half4 tangent : TANGENT;
                float2 uv : TEXCOORD0;
            };


            struct v2f
            {
                float4 positionCS : SV_POSITION;
                float2 uv : TEXCOORD0;
                float3 positionWS : TEXCOORD1;
                float3 positionVS : TEXCOORD2;
                float3 normalWS : TEXCOORD3;
                float3 tangentWS : TEXCOORD4;
                float3 bitangentWS : TEXCOORD5;
            };



            v2f vertex(appdata v, uint VertexIndex : SV_VertexID)
            {
                v2f o;
                VertexPositionInputs vertex_position_inputs = GetVertexPositionInputs(v.vertex.xyz);
                o.positionCS = vertex_position_inputs.positionCS;
                o.positionWS = vertex_position_inputs.positionWS;
                o.positionVS = vertex_position_inputs.positionVS;
                // o.uv = TRANSFORM_TEX(v.uv, _DiffuseTex);
                o.uv = v.uv;

                VertexNormalInputs normalInputs = GetVertexNormalInputs(v.normal, v.tangent);
                o.tangentWS = normalInputs.tangentWS;
                o.bitangentWS = normalInputs.bitangentWS;
                o.normalWS = normalInputs.normalWS;

                return o;
            }

            half4 frag(v2f i, bool IsFacing : SV_IsFrontFace) : SV_TARGET
            {
                const float4 _Specular = float4(1.0, 1.0, 1.0, 1.0);
                const float _Gloss = 20.f;
                Light light = GetMainLight(); /// TODO pass i.shadowCoord

                #if defined(_NORMALMAP)

                    float4 normalMap = AN_SAMPLE_TEX2D_SAMPLER(_NormalMap, _DiffuseTex, i.uv);
                    // float3 normalTS = float3(normalMap.ag * 2 - 1, 0);
                    float3 normalTS = UnpackNormalRGBNoScale(normalMap);
                    //normalTS.xy *= _NormalScale;
                    normalTS.z = sqrt(1 - dot(normalTS.xy, normalTS.xy));
                    float3 N = normalize(mul(normalTS, float3x3(i.tangentWS, i.bitangentWS, i.normalWS)));

                #else

                    float3 N = normalize(i.normalWS);

                #endif


                float3 V = normalize(mul((float3x3)AN_MATRIX_I_V, i.positionVS * (-1)));
                float3 L = normalize(light.direction);
                float3 H = normalize(L + V);

                float NoH = dot(N, H);

                // half3 ambient = half3(unity_SHAr.w, unity_SHAg.w, unity_SHAb.w);
                half3 ambient = _GlossyEnvironmentColor.xyz;


                float lambert = saturate(dot(N, L));
                float half_lambert = dot(N, L) * 0.5 + 0.5;

                float3 baseColor = AN_SAMPLE_TEX2D(_DiffuseTex, i.uv).rgb;


                float3 diffuse = light.color * baseColor * half_lambert * light.distanceAttenuation;
                diffuse = lerp(diffuse * ambient, diffuse, light.shadowAttenuation);

                // float3 reflect_dir = normalize(reflect(-L, i.normalWS));
                // float3 specular = light.color * _Specular.rgb * pow(saturate(dot(reflect_dir, view_dir)), _Gloss);

                // blinn phong
                float3 specular = light.color * _Specular.rgb * pow(saturate(NoH), _Gloss) *
                light.distanceAttenuation * light.shadowAttenuation;


                #if _ADD_LIGHT_ON
                    uint pixelLightCount = GetAdditionalLightsCount();
                    for (uint lightIndex = 0u; lightIndex < pixelLightCount; ++lightIndex)
                    {
                        Light additional_light = GetAdditionalLight(lightIndex, i.positionWS, i.shadowCoord);
                        // #if defined(_SCREEN_SPACE_OCCLUSION)
                        // light.color *= aoFactor.directAmbientOcclusion;
                        // #endif
                        half_lambert = dot(N, normalize(additional_light.direction)) * 0.5 + 0.5;
                        diffuse += (additional_light.color * baseColor * half_lambert) *
                        additional_light.distanceAttenuation * additional_light.shadowAttenuation;
                        specular += (additional_light.color * _Specular.rgb * pow(saturate(NoH), _Gloss)) *
                        additional_light.distanceAttenuation * additional_light.shadowAttenuation;
                    }
                #endif

                half3 colorOut = diffuse + ambient + specular;
                // colorOut = MixFog(colorOut, i.fogCoord);

                return half4(colorOut, 1.0);
            }
            ENDHLSL
        }
    }
}