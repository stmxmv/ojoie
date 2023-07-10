Shader "AN/SimpleShader"
{
    Properties
    {
        _DiffuseTex ("Diffuse Tex", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_DiffuseTex);
            float4 _DiffuseTex_ST;
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


            struct appdata
            {
                float3 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };


            struct v2f
            {
                float4 positionCS : SV_POSITION;
                float2 uv : TEXCOORD0;
            };



            v2f vertex(appdata v, uint VertexIndex : SV_VertexID)
            {
                v2f o;
                VertexPositionInputs vertex_position_inputs = GetVertexPositionInputs(v.vertex.xyz);
                o.positionCS = vertex_position_inputs.positionCS;
                //o.positionCS = mul(mul(mul(float4(v.vertex, 1.f), AN_MATRIX_M), AN_MATRIX_V), AN_MATRIX_P);
                o.uv = v.uv;
                return o;
            }

            half4 frag(v2f i, bool IsFacing : SV_IsFrontFace) : SV_TARGET
            {
                float3 baseColor = AN_SAMPLE_TEX2D(_DiffuseTex, i.uv).rgb;
                return half4(baseColor, 1.f);
            }
            ENDHLSL
        }
    }
}