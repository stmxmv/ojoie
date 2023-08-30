Shader "Hidden/SceneViewSelected"
{
    Properties
    {
        _MainTex ("Main Texture", 2D) = "white" {}
        _Cutoff ("Alpha cutoff", Range(0,1)) = 0.01
        _OutlineColor ("Outline Color", Color) = (1.0, 0.13, 0.0, 0.0)
        _OutlineFade ("Outline Fade", Range(0,1)) = 1.0
        _BlurDirection ("Blur Direction", Vector) = (1.0, 0.0, 0.0, 0.0)
        _TargetTexelSize ("Target Texel Size", Vector) = (0.0, 0.0, 0.0, 0.0)
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"


        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            Cull Off
            ZTest Always
            ZWrite Off

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry


            struct appdata
            {
                float3 vertex : POSITION;
            };


            struct v2f
            {
                float4 vertexOut : SV_POSITION;
            };



            v2f vertex(appdata v)
            {
                v2f o;
                VertexPositionInputs vertex_position_inputs = GetVertexPositionInputs(v.vertex.xyz);
                o.vertexOut = vertex_position_inputs.positionCS;
                return o;
            }

            half4 frag(v2f i) : SV_TARGET
            {
                return half4(1.0, 1.0, 1.0, 1.f);
            }
            ENDHLSL
        }

        Pass
        {
            Cull Off
            ZTest Always
            ZWrite Off

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry


            CBUFFER_START(ANPerMaterial)
                AN_DECLARE_TEX2D(_MainTex);
                float4 _TargetTexelSize;
                float4 _BlurDirection;
            CBUFFER_END

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };


            struct v2f
            {
                float4 vertexOut : SV_POSITION;
                float2 uv : TEXCOORD0;
            };

            static const half4 kCurveWeights[9] = {
                half4(0,0.0204001988,0.0204001988,0),
                half4(0,0.0577929595,0.0577929595,0),
                half4(0,0.1215916882,0.1215916882,0),
                half4(0,0.1899858519,0.1899858519,0),
                half4(1,0.2204586031,0.2204586031,1),
                half4(0,0.1899858519,0.1899858519,0),
                half4(0,0.1215916882,0.1215916882,0),
                half4(0,0.0577929595,0.0577929595,0),
                half4(0,0.0204001988,0.0204001988,0)
            };


            v2f vertex(appdata v)
            {
                v2f o;
                o.vertexOut = v.vertex;
                o.uv = v.uv;
                return o;
            }

            half4 frag(v2f i) : SV_TARGET
            {
                float2 step = _TargetTexelSize.xy * _BlurDirection.xy;
                float2 uv = i.uv - step * 4;
                half4 col = 0;
                for (int tap = 0; tap < 9; ++tap)
                {
                    col += AN_SAMPLE_TEX2D(_MainTex, uv) * kCurveWeights[tap];
                    uv += step;
                }
                return col;
            }
            ENDHLSL
        }

        Pass
        {
            Cull Off
            ZTest Always
            ZWrite Off
            Blend SrcAlpha OneMinusSrcAlpha, SrcAlpha OneMinusSrcAlpha
            BlendOp Add

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry


            CBUFFER_START(ANPerMaterial)
                AN_DECLARE_TEX2D(_MainTex);
                float4 _MainTex_TexelSize;
                float4 _OutlineColor;
                float _OutlineFade;
            CBUFFER_END

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };


            struct v2f
            {
                float4 vertexOut : SV_POSITION;
                float2 uv : TEXCOORD0;
            };

            v2f vertex(appdata v)
            {
                v2f o;
                o.vertexOut = v.vertex;
                o.uv = v.uv;
                return o;
            }

            half4 frag(v2f i) : SV_TARGET
            {
                half4 col = AN_SAMPLE_TEX2D(_MainTex, i.uv);

                bool isSelected = col.a > 0.9;
                float alpha = saturate(col.b * 10);
                if (isSelected)
                {
                    // outline color alpha controls how much tint the whole object gets
                    alpha = _OutlineColor.a;
                    if (any(i.uv - _MainTex_TexelSize.xy*2 < 0) || any(i.uv + _MainTex_TexelSize.xy*2 > 1))
                        alpha = 1;
                }
                bool inFront = col.g > 0.0;
                if (!inFront)
                {
                    alpha *= 0.3;
                    if (isSelected) // no tinting at all for occluded selection
                        alpha = 0;
                }
                alpha *= _OutlineFade;
                float4 outlineColor = float4(_OutlineColor.rgb,alpha);
                return outlineColor;
            }
            ENDHLSL
        }
    }
}