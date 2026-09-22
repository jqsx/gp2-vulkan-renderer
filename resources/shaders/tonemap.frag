#version 450

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D uHDRColor;

vec3 ACESFilmToneMapping(in vec3 color) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0f, 1.0f);
}

float CalculateEV100FromPhyiscalCamera(in float aperture, in float shutterTime, in float ISO) {
    return log2(pow(aperture, 2) / shutterTime * 100 / ISO);
}

float ConvertEV100ToExposure(in float EV100) {
    const float maxLum = 1.2f * pow(2.0f, EV100);
    return 1.0f / max(maxLum, 0.0001f);
}

float CalculateEV100FromAverageLum(in float avgLum) {
    const float K = 12.5f;
    return log2((avgLum * 100.0f) / K);
}

vec3 ReinhardToneMapping(in vec3 hdrColor) {
    return hdrColor / (hdrColor + vec3(1.0));
}

vec3 CompressColor(vec3 hdrColor) {
    return ACESFilmToneMapping(hdrColor);
}

void main() {

    #ifdef SUNNY
    const float aperture = 5.0f;
    const float ISO = 100.0f;
    const float shutterSpeed = 1.0f / 200.0f;
    #else
    const float aperture = 1.4f;
    const float ISO = 1600.0f;
    const float shutterSpeed = 1.0f / 60.0f;
    #endif

    const float physicalCamera = CalculateEV100FromPhyiscalCamera(aperture, shutterSpeed, ISO);
    const float exposure = ConvertEV100ToExposure(physicalCamera);

    vec3 hdrColor = texture(uHDRColor, vTexCoord).rgb;
    fragColor = vec4(CompressColor(hdrColor * exposure), 1.0);
}
