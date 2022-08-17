#version 450

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;
} fragmentIn;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 1) uniform LightContext {
    vec4 color;
    vec3 lightPos;
    vec3 lightColor;
} ubo;



void main() {

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;

    // diffuse
    vec3 norm = normalize(fragmentIn.Normal);
    vec3 lightDir = normalize(ubo.lightPos - fragmentIn.worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    vec4 result = vec4(ambient + diffuse, 1.f) * ubo.color;

    if (result.a < 0.1f) {
        discard;
    }

    FragColor = vec4(result);
}
