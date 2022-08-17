#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;
} fragmentIn;

layout (set = 0, binding = 1) uniform LightContext {
    vec3 lightPos;
    vec3 lightColor;
} ubo;

layout (set = 0, binding = 2) uniform sampler colorSampler;

layout (set = 0, binding = 3) uniform texture2D texture_diffuse1;


void main() {

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;

    // diffuse
    vec3 norm = normalize(fragmentIn.Normal);
    vec3 lightDir = normalize(ubo.lightPos - fragmentIn.worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    vec4 sampleColor = texture(sampler2D(texture_diffuse1, colorSampler), fragmentIn.TexCoord);

    vec4 result = vec4(ambient + diffuse, 1.f) * sampleColor;
    if (result.a < 0.1f) {
        discard;
    }
    FragColor = result;
}
