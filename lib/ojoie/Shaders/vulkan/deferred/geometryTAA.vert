#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;

    vec4 preScreenPosition;
    vec4 nowScreenPosition;
} vertexOut;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat3 normalMatrix;
} ubo;

const vec2 Halton_2_3[8] =  {
    vec2(0.0f, -1.0f / 3.0f),
    vec2(-1.0f / 2.0f, 1.0f / 3.0f),
    vec2(1.0f / 2.0f, -7.0f / 9.0f),
    vec2(-3.0f / 4.0f, -1.0f / 9.0f),
    vec2(1.0f / 4.0f, 5.0f / 9.0f),
    vec2(-1.0f / 4.0f, -5.0f / 9.0f),
    vec2(3.0f / 4.0f, 1.0f / 9.0f),
    vec2(-7.0f / 8.0f, 7.0f / 9.0f)
};

layout (set = 1, binding = 0) uniform TaaUniform {
    float screenWidth, screenHeight;
    int offsetIdx;
    mat4 preProjection;
    mat4 preView;
    mat4 preModel;
} taaUniform;

void main() {

    float deltaWidth = 1.0 / taaUniform.screenWidth, deltaHeight = 1.0 / taaUniform.screenHeight;
    vec2 jitter = vec2(
        Halton_2_3[taaUniform.offsetIdx % 8].x * deltaWidth,
        Halton_2_3[taaUniform.offsetIdx % 8].y * deltaHeight
    );
    mat4 jitterMat = ubo.projection;
    jitterMat[2][0] += jitter.x;
    jitterMat[2][1] += jitter.y;

    gl_Position = jitterMat * ubo.view * ubo.model * vec4(aPos, 1.0);

    vertexOut.TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    vertexOut.Normal = ubo.normalMatrix * aNormal;
    vec4 position = ubo.model * vec4(aPos, 1.0);
    vertexOut.worldPos = vec3(position.x / position.w, position.y / position.w, position.z / position.w);

    vertexOut.preScreenPosition = taaUniform.preProjection * taaUniform.preView * taaUniform.preModel * vec4(aPos, 1.0);
    vertexOut.nowScreenPosition = ubo.projection * ubo.view * ubo.model * vec4(aPos, 1.0);
}
