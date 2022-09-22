#version 450
layout (location = 0) in vec2 TexCoords;

layout (location = 0) out vec4 colorOut;

layout (push_constant) uniform uPushConstant {
    float width;
    float edge;
    vec4 color;
    mat4 projection;
} pc;

layout (set = 0, binding = 0) uniform sampler textSampler;
layout (set = 0, binding = 2) uniform texture2D text;



void main() {

    float distance = 1.f - texture(sampler2D(text, textSampler), TexCoords).r;
    float alpha = 1.f - smoothstep(pc.width, pc.width + pc.edge, distance);

    colorOut = vec4(pc.color.rgb, pc.color.a * alpha);
}