#version 450 core

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;

    vec4 preScreenPosition;
    vec4 nowScreenPosition;
} fragmentIn;

layout (location = 0) out vec4 albedoOut;
layout (location = 1) out vec4 normalOut;
layout (location = 2) out vec2 velocityOut;

layout (set = 0, binding = 2) uniform sampler colorSampler;

layout (set = 0, binding = 3) uniform texture2D baseColorTexture;



void main() {

    vec3 normal = normalize(fragmentIn.normal);
    // Transform normals from [-1, 1] to [0, 1]


    vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

    base_color = texture(sampler2D(baseColorTexture, colorSampler), fragmentIn.texCoord);

    if (base_color.a < 0.1f) {
        discard;
    }

    normalOut = vec4(0.5 * normal + 0.5, base_color.a);

    albedoOut = base_color;

    // velocity
    vec2 newPos = ((fragmentIn.nowScreenPosition.xy / fragmentIn.nowScreenPosition.w) * 0.5 + 0.5);
    vec2 prePos = ((fragmentIn.preScreenPosition.xy / fragmentIn.preScreenPosition.w) * 0.5 + 0.5);
    velocityOut = newPos - prePos;
}