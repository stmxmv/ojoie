#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in struct {
    vec3 worldPos;
    vec2 TexCoord;
    vec3 Normal;
} fragmentIn;

layout (set = 0, binding = 1) uniform LightContext {
    vec4 lightPos;
    vec4 lightColor;
} ubo;

layout (set = 0, binding = 2) uniform sampler colorSampler;

layout (set = 0, binding = 3) uniform texture2D texture_diffuse1;


vec3 apply_point_light(vec3 lightPos, vec3 worldPos, vec4 color, vec3 normal)  {
    vec3 world_to_light = lightPos - worldPos;

    float dist = length(world_to_light) * 0.005;

    float atten = 1.0 / (dist * dist);

    world_to_light = normalize(world_to_light);

    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

    return ndotl * atten * color.w * color.rgb;
}

void main() {

    vec4 sampleColor = texture(sampler2D(texture_diffuse1, colorSampler), fragmentIn.TexCoord);
    if (sampleColor.a < 0.1f) {
        discard;
    }

    vec3 normal = normalize(fragmentIn.Normal);

    vec3 lightness = apply_point_light(ubo.lightPos.xyz, fragmentIn.worldPos, ubo.lightColor, normal);

    vec3 ambient_color = vec3(0.2) * sampleColor.xyz;

    FragColor = vec4(ambient_color + lightness * sampleColor.xyz, 1.0);
}
