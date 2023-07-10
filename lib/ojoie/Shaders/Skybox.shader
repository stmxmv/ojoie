Shader "AN/Skybox"
{
    Properties
    {
        _MainTex("src tex", Cube) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"

        CBUFFER_START(ANPerMaterial)
            TextureCube _MainTex;
            SamplerState sampler_MainTex;
        CBUFFER_END

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


			struct appdata_t {
				float4 vertex : POSITION;
			};

			struct v2f {
				float4 vertex : SV_Position;
				float3 texcoord : TEXCOORD0;
			};



            v2f vertex(appdata_t v, uint VertexIndex : SV_VertexID)
            {
				v2f o;
                float3 posWorld = mul(an_ObjectToWorld, float4(v.vertex.xyz, 1.0)).xyz;
                o.vertex = mul(AN_MATRIX_VP, float4(posWorld, 1.0));
				o.texcoord = normalize(v.vertex.xyz);
				return o;
            }

            half4 frag(v2f i) : SV_TARGET
            {
                return _MainTex.Sample(sampler_MainTex, i.texcoord);
            }
            ENDHLSL
        }
    }
}