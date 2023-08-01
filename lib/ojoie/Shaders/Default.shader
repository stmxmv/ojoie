Shader "AN/Default"
{
    Properties
    {
        _MainColor ("Main Color", Color) = (1.0, 1.0, 1.0, 1.0)
        _DiffuseTex ("Diffuse Tex", 2D) = "white" {}
        _Specular ("Specular", Color) = (1.0, 1.0, 1.0, 1.0)
        _Gloss ("Gloss", Range(1, 256)) = 20
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"
        #include "Lighting.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_DiffuseTex);
            float4 _MainColor;
            float4 _Specular;
            float _Gloss;
        CBUFFER_END

        ENDHLSL

        Pass 
        {
            Tags { "LightMode" = "ShadowCaster" }

            HLSLPROGRAM

            struct appdata 
            {   
                float3 vertex : POSITION;
            };

            struct v2f
            {
                float4 vertexOut : SV_POSITION;
            };

            v2f vertex_main(appdata v)
            {
                v2f o;
                o.vertexOut = TransformWorldToHClip(v.vertex.xyz);
                return o;
            }

            half4 fragment_main(v2f i) : SV_TARGET 
            {
                return half4(0.0, 0.0, 0.0, 0.0);
            }


            ENDHLSL
        }

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


            struct appdata
            {
                float3 vertex : POSITION;
                float2 uv : TEXCOORD0;
                float3 normal : NORMAL;
            };


            struct v2f
            {
                float4 positionCS : SV_POSITION;
                float3 positionVS : TEXCOORD0;
                float2 uv : TEXCOORD1;
                float3 normalWS : TEXCOORD2;
            };



            v2f vertex(appdata v, uint VertexIndex : SV_VertexID)
            {
                v2f o;
                VertexPositionInputs vertex_position_inputs = GetVertexPositionInputs(v.vertex.xyz);
                o.positionCS = vertex_position_inputs.positionCS;
                o.positionVS = vertex_position_inputs.positionVS;
                //o.positionCS = mul(mul(mul(float4(v.vertex, 1.f), AN_MATRIX_M), AN_MATRIX_V), AN_MATRIX_P);
                o.uv = v.uv;

                VertexNormalInputs normalInputs = GetVertexNormalInputs(v.normal);
                o.normalWS = normalInputs.normalWS;

                return o;
            }

            half4 frag(v2f i, bool IsFacing : SV_IsFrontFace) : SV_TARGET
            {
                Light light = GetMainLight();
                float3 N = normalize(i.normalWS);
                float3 V = normalize(mul((float3x3)AN_MATRIX_I_V, i.positionVS * (-1)));
                float3 L = normalize(light.direction);
                float3 H = normalize(L + V);
                float NoH = dot(N, H);

                half3 ambient = _GlossyEnvironmentColor.xyz;
                float half_lambert = dot(N, L) * 0.5 + 0.5;


                float3 baseColor = AN_SAMPLE_TEX2D(_DiffuseTex, i.uv).rgb;

                float3 diffuse = light.color * baseColor * half_lambert * light.distanceAttenuation;
                //diffuse = lerp(diffuse * ambient, diffuse, light.shadowAttenuation);
                float3 specular = light.color * _Specular.rgb * pow(saturate(NoH), _Gloss) *
                                  light.distanceAttenuation * light.shadowAttenuation;

                half3 colorOut = diffuse + ambient + specular;

                return half4(colorOut, 1.0) * _MainColor;
            }
            ENDHLSL
        }
    }
}