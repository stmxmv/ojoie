Shader "Hidden/SceneViewSelection"
{
    Properties
    {

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

            Cull Back
            ZTest LEqual
            ZWrite On

            HLSLPROGRAM

            #define vertex vertex_main // define vertex shader entry
            #define frag fragment_main // define fragment shader entry

            CBUFFER_START(ANPerMaterial)
                float4 _SelectionID;
            CBUFFER_END

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
                return _SelectionID;
            }
            ENDHLSL
        }
    }
}