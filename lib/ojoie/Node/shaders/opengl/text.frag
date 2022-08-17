#version 430 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec4 textColor;

uniform float width;
uniform float edge;

void main() {

    float distance = 1.f - texture(text, TexCoords).r;
    float alpha = 1.f - smoothstep(width, width + edge, distance);

    color = vec4(textColor.rgb, textColor.a * alpha);
}