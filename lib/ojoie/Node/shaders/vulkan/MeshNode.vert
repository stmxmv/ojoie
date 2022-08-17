#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;


layout (location = 0) out struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;
} vertexOut;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
} ubo;



void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(aPos, 1.0);
    vertexOut.TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    vertexOut.Normal = mat3(ubo.normalMatrix) * aNormal;
    vertexOut.worldPos = vec3(ubo.model * vec4(aPos, 1.0));
}
