#version 430 core

out vec4 FragColor;

in vec3 worldPos;
in vec2 TexCoord;
in vec3 Normal;

uniform vec4 color;


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

    vec4 result = vec4(ambient + diffuse, 1.f) * color;

    if (result.a < 0.1f) {
        discard;
    }

    FragColor = vec4(result);
}
