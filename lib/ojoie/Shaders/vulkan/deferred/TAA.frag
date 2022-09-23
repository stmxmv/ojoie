#version 450 core

layout(location = 0) in vec2 uvIn;
layout(location = 0) out vec4 colorOut;

layout (set = 0, binding = 0) uniform sampler colorSampler;
layout (set = 0, binding = 1) uniform texture2D depth;
layout (set = 0, binding = 2) uniform texture2D velocity;
layout (set = 0, binding = 3) uniform texture2D currentColor;
layout (set = 0, binding = 4) uniform texture2D previousColor;

layout (push_constant) uniform TaaUniform {
    float screenWidth, screenHeight;
} taaUniform;

vec3 RGB2YCoCgR(vec3 rgbColor) {
    vec3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2;

    return YCoCgRColor;
}

vec3 YCoCgR2RGB(vec3 YCoCgRColor)  {
    vec3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

float Luminance(vec3 color)  {
    return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

vec3 ToneMap(vec3 color)  {
    return color / (1 + Luminance(color));
}

vec3 UnToneMap(vec3 color)  {
    return color / (1 - Luminance(color));
}

vec2 getClosestOffset() {
    vec2 deltaRes = vec2(1.0 / taaUniform.screenWidth, 1.0 / taaUniform.screenHeight);
    float closestDepth = 1.0f;
    vec2 closestUV = uvIn;

    for (int i = -1;i <= 1; ++i)  {
        for (int j = -1;j <= 1; ++j)  {
            vec2 newUV = uvIn + deltaRes * vec2(i, j);

            float depth = texture(sampler2D(depth, colorSampler), newUV).x;

            if (depth < closestDepth)  {
                closestDepth = depth;
                closestUV = newUV;
            }
        }
    }

    return closestUV;
}

vec3 clipAABB(vec3 nowColor, vec3 preColor)  {
    vec3 aabbMin = nowColor, aabbMax = nowColor;
    vec2 deltaRes = vec2(1.0 / taaUniform.screenWidth, 1.0 / taaUniform.screenHeight);
    vec3 m1 = vec3(0), m2 = vec3(0);

    for (int i = -1;i <= 1; ++i) {
        for (int j = -1;j <= 1; ++j) {
            vec2 newUV = uvIn + deltaRes * vec2(i, j);
            vec3 C = RGB2YCoCgR(ToneMap(texture(sampler2D(currentColor, colorSampler), newUV).rgb));
            m1 += C;
            m2 += C * C;
        }
    }

    // Variance clip
    const int N = 9;
    const float VarianceClipGamma = 1.0f;
    vec3 mu = m1 / N;
    vec3 sigma = sqrt(abs(m2 / N - mu * mu));
    aabbMin = mu - VarianceClipGamma * sigma;
    aabbMax = mu + VarianceClipGamma * sigma;

    // clip to center
    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin);

    vec3 v_clip = preColor - p_clip;
    vec3 v_unit = v_clip.xyz / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0) {
        return p_clip + v_clip / ma_unit;
    } else {
        return preColor;
    }
}

void main() {
    vec3 nowColor = texture(sampler2D(currentColor, colorSampler), uvIn).rgb;

    // closet velocity among 3x3 area
    vec2 velocity = texture(sampler2D(velocity, colorSampler), getClosestOffset()).rg;
    vec2 offsetUV = clamp(uvIn - velocity, 0, 1);
    vec3 preColor = texture(sampler2D(previousColor, colorSampler), offsetUV).rgb;

    nowColor = RGB2YCoCgR(ToneMap(nowColor));
    preColor = RGB2YCoCgR(ToneMap(preColor));

    preColor = clipAABB(nowColor, preColor);

    preColor = UnToneMap(YCoCgR2RGB(preColor));
    nowColor = UnToneMap(YCoCgR2RGB(nowColor));

    // mix
    const float c = 0.05;
    colorOut = vec4(c * nowColor + (1 - c) * preColor, 1.0);
}