#version 450 core

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;
} fragmentIn;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 1) uniform LightContext {
    vec4 lightPos;
    vec4 lightColor;
} ubo;

layout(push_constant) uniform PBRMaterialUniform {
    vec4 baseColorFactor;
} pbrMaterialUniform;

vec3 apply_point_light(vec3 lightPos, vec3 worldPos, vec4 color, vec3 normal)  {
    vec3 world_to_light = lightPos - worldPos;

    float dist = length(world_to_light) * 0.005;

    float atten = 1.0 / (dist * dist);

    world_to_light = normalize(world_to_light);

    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

    return ndotl * atten * color.w * color.rgb;
}

void main() {

    // diffuse
    vec3 normal = normalize(fragmentIn.Normal);
    vec3 lightness = apply_point_light(ubo.lightPos.xyz, fragmentIn.worldPos, ubo.lightColor, normal);

    vec3 ambient_color = vec3(0.2) * pbrMaterialUniform.baseColorFactor.rgb;

    FragColor = vec4(ambient_color + lightness * pbrMaterialUniform.baseColorFactor.rgb, 1.0);
}
