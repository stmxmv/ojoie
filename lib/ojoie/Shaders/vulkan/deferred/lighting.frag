#version 450

layout(location = 0) in vec2 uvIn;

layout(location = 0) out vec4 colorOut;

layout(input_attachment_index = 0, binding = 0) uniform subpassInput depth;
layout(input_attachment_index = 1, binding = 2) uniform subpassInput albedo;
layout(input_attachment_index = 2, binding = 3) uniform subpassInput normal;

layout(push_constant) uniform GlobalUniform  {
    mat4 inv_view_proj;
    vec2 inv_resolution;
}  globalUniform;

layout (set = 0, binding = 1) uniform LightContext {
    vec4 lightPos;   // position.w represents type of light
    vec4 lightColor; // color.w represents light intensity
} lights;

vec3 apply_point_light(vec3 lightPos, vec3 worldPos, vec4 color, vec3 normal)  {
    vec3 world_to_light = lightPos - worldPos;

    float dist = length(world_to_light) * 0.005;

    float atten = 1.0 / (dist * dist);

    world_to_light = normalize(world_to_light);

    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

    return ndotl * atten * color.w * color.rgb;
}

void main() {

    // Retrieve position from depth
    vec4  clip         = vec4(gl_FragCoord.xy * globalUniform.inv_resolution * 2.0 - 1.0, subpassLoad(depth).x, 1.0);
    vec4 world_w = globalUniform.inv_view_proj * clip;
    vec3 pos     = world_w.xyz / world_w.w;

    vec4 albedo = subpassLoad(albedo);
    // Transform back from [0,1] to [-1,1]
    vec3 normal = subpassLoad(normal).xyz;
    normal      = normalize(2.0 * normal - 1.0);


    vec3 lightness = apply_point_light(lights.lightPos.xyz, pos, lights.lightColor, normal);

    vec3 ambient_color = vec3(0.2) * albedo.xyz;

    colorOut = vec4(ambient_color + lightness * albedo.xyz, 1.0);
}