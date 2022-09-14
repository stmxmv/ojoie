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

layout (set = 0, binding = 4) uniform texture2D texture_diffuse2;


void main() {

    vec4 sampleColor = texture(sampler2D(texture_diffuse1, colorSampler), fragmentIn.TexCoord);
    if (sampleColor.a < 0.1f) {
        discard;
    }

    vec3 normal;
    /// flip normal if needed
    if (dot(fragmentIn.Normal, fragmentIn.worldPos) >= 0.f) {
        normal = normalize(-fragmentIn.Normal);
    } else {
        normal = normalize(fragmentIn.Normal);
    }

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;

    // diffuse
    vec3 lightDir = normalize(ubo.lightPos - fragmentIn.worldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    vec4 result = vec4(ambient + diffuse, 1.f) * sampleColor;

    FragColor = result;
}
