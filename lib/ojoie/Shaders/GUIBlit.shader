Shader "AN/BlitCopy"
{
    Properties
    {
        _MainTex("src tex", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_MainTex);
        CBUFFER_END

        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            Blend SrcAlpha OneMinusSrcAlpha, One One
            BlendOp Add

            Cull Off
            ZTest Always
            ZWrite Off

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry


			struct appdata_t {
				float4 vertex : POSITION;
				float2 texcoord : TEXCOORD0;
			};

			struct v2f {
				float4 vertex : SV_Position;
				float2 texcoord : TEXCOORD0;
			};



            v2f vertex(appdata_t v, uint VertexIndex : SV_VertexID)
            {
				v2f o;
				o.vertex = v.vertex;
				o.texcoord = v.texcoord.xy;
				return o;
            }

            half4 frag(v2f i) : SV_TARGET
            {
                return AN_SAMPLE_TEX2D(_MainTex, i.texcoord);
            }
            ENDHLSL
        }
    }
}