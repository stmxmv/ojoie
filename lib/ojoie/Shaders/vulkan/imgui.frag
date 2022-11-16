#version 450 core
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler colorSampler;

layout(set = 0, binding = 2) uniform texture2D currentTexture;

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
} In;

void main() {
    /// gamma correct In.Color to linear space
    const float gamma = 2.2;
    vec4 InColor = vec4(pow(In.color.rgb, vec3(gamma)), In.color.a);

    vec4 resultColor = InColor * texture(sampler2D(currentTexture, colorSampler), In.uv.st);

    outColor = resultColor;
}

// glslangValidator -V -x -o imgui.frag.u32 imgui.frag