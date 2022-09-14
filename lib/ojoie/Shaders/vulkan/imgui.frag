#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding = 0) uniform sampler colorSampler;

layout(set=0, binding = 1) uniform texture2D fontTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main() {
    vec4 resultColor = In.Color * texture(sampler2D(fontTexture, colorSampler), In.UV.st);

    /// gamma correction to linear space
    const float gamma = 2.2;
    vec3 correctColor = pow(resultColor.rgb, vec3(gamma));

    fColor = vec4(correctColor, resultColor.a);
}

// glslangValidator -V -x -o imgui.frag.u32 imgui.frag