#version 450

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;
} fragmentIn;

layout (location = 0) out vec4 albedoOut;
layout (location = 1) out vec4 normalOut;


layout (set = 0, binding = 2) uniform sampler colorSampler;

layout (set = 0, binding = 3) uniform texture2D baseColorTexture;


void main() {

    vec3 normal = normalize(fragmentIn.normal);
    // Transform normals from [-1, 1] to [0, 1]
    normalOut = vec4(0.5 * normal + 0.5, 1.0);

    vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

    base_color = texture(sampler2D(baseColorTexture, colorSampler), fragmentIn.texCoord);

    if (base_color.a < 0.1f) {
        discard;
    }
    
    albedoOut = base_color;

}