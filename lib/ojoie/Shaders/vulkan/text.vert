#version 450
layout (location = 0) in vec4 posTex; // <vec2 pos, vec2 tex>

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out vec2 TexCoords;

layout (push_constant) uniform uPushConstant {
    float width;
    float edge;
    vec4 color;
    mat4 projection;
} pc;


void main() {
    gl_Position = pc.projection * vec4(posTex.xy, 0.0, 1.0);
    TexCoords = posTex.zw;
}