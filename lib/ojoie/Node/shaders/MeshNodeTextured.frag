#version 430 core

out vec4 FragColor;

in vec3 worldPos;
in vec2 TexCoord;
in vec3 Normal;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_diffuse2;
    sampler2D texture_diffuse3;
    sampler2D texture_specular1;
    sampler2D texture_specular2;
};

uniform Material material;

uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec4 sampleColor = texture(material.texture_diffuse1, TexCoord);

    vec4 result = vec4(ambient + diffuse, 1.f) * sampleColor;
    if (result.a < 0.1f) {
        discard;
    }
    FragColor = result;
}
