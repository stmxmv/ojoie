#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out struct {
    vec3 worldPos;
    vec3 positionVS;
    vec2 TexCoord;
    vec3 Normal;
} vertexOut;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat3 normalMatrix;
} ubo;



void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(aPos, 1.0);
    vertexOut.TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    vertexOut.Normal = ubo.normalMatrix * aNormal;
    vec4 position = ubo.model * vec4(aPos, 1.0);
    vertexOut.worldPos = vec3(position.x / position.w, position.y / position.w, position.z / position.w);

    vec4 positionVS = ubo.view * vec4(aPos, 1.0);
    vertexOut.positionVS = vec3(positionVS.x / positionVS.w, positionVS.y / positionVS.w, positionVS.z / positionVS.w);
}
