#ifndef AN_INPUT_HLSL
#define AN_INPUT_HLSL


#define RENDERING_LIGHT_LAYERS_MASK (255)
#define RENDERING_LIGHT_LAYERS_MASK_SHIFT (0)
#define DEFAULT_LIGHT_LAYERS (RENDERING_LIGHT_LAYERS_MASK >> RENDERING_LIGHT_LAYERS_MASK_SHIFT)


cbuffer ANGlobal
{

    // x = 1 or -1 (-1 if projection is flipped)
    // y = near plane
    // z = far plane
    // w = 1/far plane
    float4 _ProjectionParams;

    float4x4 an_MatrixV;
    float4x4 an_MatrixInvV;
    float4x4 an_MatrixP;
    float4x4 an_MatrixInvP;
    float4x4 an_MatrixVP;
    float4x4 an_MatrixInvVP;

    float4 _GlossyEnvironmentColor;

    float4 _MainLightPosition;
    float4 _MainLightColor;
    uint _MainLightLayerMask;

    // Light Indices block feature
    // These are set internally by the engine upon request by RendererConfiguration.
    float4 an_LightData;
}



// Block Layout should be respected due to SRP Batcher
cbuffer ANPerDraw {

    float4 an_WorldTransformParams; // w is usually 1.0, or -1.0 for odd-negative scale transforms

    // Space block Feature
    float4x4 an_ObjectToWorld;
    float4x4 an_WorldToObject;

    // Velocity
    //float4x4 an_MatrixPreviousM;
    //float4x4 an_MatrixPreviousMI;
    //X : Use last frame positions (right now skinned meshes are the only objects that use this
    //Y : Force No Motion
    //Z : Z bias value
    //W : Camera only
    //float4 an_MotionVectorsParams;

}

#define AN_MATRIX_M     an_ObjectToWorld
#define AN_MATRIX_I_M   an_WorldToObject
#define AN_MATRIX_V     an_MatrixV
#define AN_MATRIX_I_V   an_MatrixInvV
#define AN_MATRIX_P     an_MatrixP
#define AN_MATRIX_I_P   an_MatrixInvP
#define AN_MATRIX_VP    an_MatrixVP
#define AN_MATRIX_I_VP  an_MatrixInvVP


#endif//AN_INPUT_HLSL