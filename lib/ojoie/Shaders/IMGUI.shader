Shader "AN/SimpleShader"
{
    Properties
    {
        _IMGUITexture ("IMGUI Tex", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_IMGUITexture);
            float4x4 _IMGUITransform;
        CBUFFER_END

        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            Cull Off
            Blend SrcAlpha OneMinusSrcAlpha, One One
            BlendOp Add
            ColorMask RGBA
            ZTest Always
            ZWrite Off
            ZClip True

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry


            struct appdata
            {
                float2 pos : POSITION;
                float2 uv : TEXCOORD0;
                float4 color : COLOR;
            };


            struct v2f
            {
                float4 positionCS : SV_POSITION;
                float4 color : TEXCOORD0;
                float2 uv : TEXCOORD1;
            };



            v2f vertex(appdata v, uint VertexIndex : SV_VertexID)
            {
                v2f o;
                o.positionCS = mul(_IMGUITransform, float4(v.pos.xy, 0.0, 1.0));
                o.uv = v.uv;
                o.color = v.color;
                return o;
            }

            half4 frag(v2f i, bool IsFacing : SV_IsFrontFace) : SV_TARGET
            {
                /// gamma correct In.Color to linear space
                const float gamma = 2.2;
                float4 inColor = float4(pow(i.color.rgb, float3(gamma, gamma, gamma)), i.color.a);
                float4 texColor = AN_SAMPLE_TEX2D(_IMGUITexture, i.uv);
                half4 resultColor = inColor * texColor;
                return resultColor;
            }
            ENDHLSL
        }
    }
}