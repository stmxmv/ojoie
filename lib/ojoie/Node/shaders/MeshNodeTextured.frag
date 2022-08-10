#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_diffuse2;
    sampler2D texture_diffuse3;
    sampler2D texture_specular1;
    sampler2D texture_specular2;
};

uniform Material material;


void main() {
    vec4 sampleColor = texture(material.texture_diffuse1, TexCoord);
    if (sampleColor.a < 0.1f) {
        discard;
    }
    FragColor = sampleColor;
}
