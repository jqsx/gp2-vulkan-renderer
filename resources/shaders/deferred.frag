#version 450

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D uColorRoughness;
layout(set = 0, binding = 1) uniform sampler2D uNormal;
layout(set = 0, binding = 2) uniform sampler2D uPosition;
layout(set = 0, binding = 3) uniform DeferredLightingData {
    mat4 lightView;
    mat4 lightProjection;
    mat4 lightViewProjection;
    mat4 inverseView;
    mat4 inverseProjection;
    vec4 lightDirection;
    vec4 cameraPosition;
} uLighting;
layout(set = 0, binding = 4) uniform sampler2D uShadowMap;
layout(set = 0, binding = 5) uniform samplerCube uSkybox;

struct PointLightShadowData {
    vec4 positionRadius;
    vec4 colorIntensity;
};

layout(std430, set = 0, binding = 6) readonly buffer PointLightShadowBuffer {
    PointLightShadowData pointLights[];
} uPointLightShadows;
layout(set = 0, binding = 7) uniform samplerCube uPointShadowMaps[8];

const float PI = 3.14;
const float MIN_ROUGHNESS = 0.04;
const float DIRECT_SPECULAR_STRENGTH = 1.0;
const float SKYBOX_SPECULAR_STRENGTH = 0.16;
const float AMBIENT_STRENGTH = 0.32;
const vec3 AMBIENT_IRRADIANCE = vec3(0.50, 0.56, 0.70);
const uint MAX_POINT_LIGHTS = 8;

const vec2 POISSON_OFFSETS[12] = vec2[12](
    vec2(-0.326, -0.406),
    vec2(-0.840, -0.074),
    vec2(-0.696,  0.457),
    vec2(-0.203,  0.621),
    vec2( 0.962, -0.195),
    vec2( 0.473, -0.480),
    vec2( 0.519,  0.767),
    vec2( 0.185, -0.893),
    vec2( 0.507,  0.064),
    vec2( 0.896,  0.412),
    vec2(-0.322, -0.933),
    vec2(-0.792, -0.598)
);

vec3 SanitizeColor(vec3 color) {
    if (any(isnan(color)) || any(isinf(color)))
    return vec3(0.0);
    return clamp(color, vec3(0.0), vec3(16.0));
}

vec3 SampleSkybox(vec3 direction) {
    return SanitizeColor(texture(uSkybox, normalize(direction)).rgb);
}

vec3 GetSkyboxDirection(vec2 uv) {
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 viewPosition = uLighting.inverseProjection * vec4(ndc, 1.0, 1.0);
    vec3 viewDirection = normalize(viewPosition.xyz / viewPosition.w);
    return normalize(mat3(uLighting.inverseView) * viewDirection);
}

float GetShadowValue(vec3 shadowCoord, vec3 normal, vec3 lightDir) {
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0) {
        return 1.0;
    } // I know this is not necessary for this setup but it still just makes sure ig

    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    float radius = 1.0 / textureSize(uShadowMap, 0).x;
    for (int index = 0; index < 12; index++) {
        float shadowDepth = texture(uShadowMap, shadowCoord.xy + POISSON_OFFSETS[index] * radius).r;

        shadow += shadowCoord.z - bias > shadowDepth ? 0.0 : 1.0;
    }
    return shadow / 12.0;
}

float DistributionGGX(vec3 normal, vec3 halfway, float roughness) {
    float r2 = roughness * roughness;
    float nDotH = max(dot(normal, halfway), 0.0);
    float nDotH2 = nDotH * nDotH;

    float denom = (nDotH2 * (r2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return r2 / max(denom, 0.000001);
}

float GeometrySchlickGGX(vec3 normal, vec3 direction, float roughness) {
    float r2 = roughness * roughness;
    float directionDotNormal = max(dot(direction, normal), 0.0);
    return directionDotNormal / max(directionDotNormal * (1.0 - r2) + r2, 0.000001);
}

float GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
    return GeometrySchlickGGX(normal, viewDir, roughness) * GeometrySchlickGGX(normal, lightDir, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 EvaluateDirectionalBRDF(vec3 albedo, float metallic, float roughness, vec3 normal, vec3 viewDir, vec3 lightDir, float geometricLightDot, float shadow) {
    vec3 halfway = normalize(viewDir + lightDir);
    vec3 radiance = vec3(1.0);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float d = DistributionGGX(normal, halfway, roughness);
    float g = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 f = FresnelSchlick(max(dot(halfway, viewDir), 0.0), f0);

    vec3 numerator = d * g * f;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0);
    vec3 specular = numerator / max(denominator, 0.000001);

    vec3 kD = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 lambertDiffuse = kD * albedo / PI;
    float nDotL = min(max(dot(normal, lightDir), 0.0), geometricLightDot);

    return (lambertDiffuse + specular * DIRECT_SPECULAR_STRENGTH) * radiance * nDotL * shadow;
}

vec3 EvaluateBRDF(vec3 albedo, float metallic, float roughness, vec3 normal, vec3 viewDir, vec3 lightDir) {
    vec3 halfway = normalize(viewDir + lightDir);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float d = DistributionGGX(normal, halfway, roughness);
    float g = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 f = FresnelSchlick(max(dot(halfway, viewDir), 0.0), f0);

    vec3 numerator = d * g * f;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0);
    vec3 specular = numerator / max(denominator, 0.000001);

    vec3 kD = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 lambertDiffuse = kD * albedo / PI;
    float nDotL = max(dot(normal, lightDir), 0.0);

    return (lambertDiffuse + specular * DIRECT_SPECULAR_STRENGTH) * nDotL;
}

float GetPointShadowValue(uint lightIndex, vec3 position, vec3 normal, vec3 lightToFragment, float radius) {
    float currentDepth = length(lightToFragment) / radius;
    vec3 fragmentToLight = normalize(-lightToFragment);
    float bias = max(0.003 * (1.0 - dot(normal, fragmentToLight)), 0.001);

    float closestDepth = texture(uPointShadowMaps[lightIndex], lightToFragment).r;
    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

vec3 EvaluatePointLights(vec3 albedo, float metallic, float roughness, vec3 normal, vec3 viewDir, vec3 position) {
    vec3 result = vec3(0.0);
    uint lightCount = min(uint(uLighting.cameraPosition.w + 0.5), MAX_POINT_LIGHTS);
    normal.x *= -1.0;

    for (uint lightIndex = 0; lightIndex < lightCount; ++lightIndex) {
        PointLightShadowData light = uPointLightShadows.pointLights[lightIndex];
        vec3 lightPosition = light.positionRadius.xyz;
        float radius = max(light.positionRadius.w, 0.01);
        // Cubemap faces are baked from the light outward, so sample with light -> fragment.
        vec3 lightToFragment = lightPosition - position;
        float distanceToLight = length(lightToFragment);

        if (distanceToLight >= radius)
            continue;

        vec3 lightDir = normalize(lightPosition - position);
        float attenuation = pow(clamp(1.0 - distanceToLight / radius, 0.0, 1.0), 2.0);
        float shadow = GetPointShadowValue(lightIndex, position, normal, lightToFragment, radius);
        float shadowVisibility = mix(0.12, 1.0, shadow);
        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation * 6.0;

        result += EvaluateBRDF(albedo, metallic, roughness, normal, viewDir, lightDir) * radiance * shadowVisibility;
    }

    return result;
}

vec3 EvaluateSkyboxAmbient(vec3 albedo, float metallic, float roughness, vec3 normal, vec3 viewDir) {
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 f = FresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), f0, roughness);
    vec3 kD = (vec3(1.0) - f) * (1.0 - metallic);

    vec3 diffuseIrradiance = AMBIENT_IRRADIANCE;
    vec3 reflection = reflect(-viewDir, normal);
    float maxReflectionLod = 32.0;
    float lod = roughness * maxReflectionLod;
    vec3 specularIrradiance = SanitizeColor(textureLod(uSkybox, normalize(reflection), lod).rgb); // Tried but it still looks like shit

    float ambientShape = mix(0.55, 1.0, clamp(normal.y * 0.5 + 0.5, 0.0, 1.0));
    float environmentSpecularVisibility = pow(clamp(1.0 - roughness, 0.0, 1.0), 2.0);
    return kD * albedo * diffuseIrradiance * AMBIENT_STRENGTH * ambientShape + specularIrradiance * f * SKYBOX_SPECULAR_STRENGTH * environmentSpecularVisibility;
}

float GetObservableArea(vec3 normal) {
    return max(dot(normal, -uLighting.lightDirection.xyz), 0.0);
}

void main() {
    vec4 colorRoughness = texture(uColorRoughness, vTexCoord);
    vec4 positionMetallic = texture(uPosition, vTexCoord);
    vec4 normalGeometric = texture(uNormal, vTexCoord);

    float roughness = max(clamp(colorRoughness.a, 0.0, 1.0), MIN_ROUGHNESS);
    float metallic = clamp(positionMetallic.a, 0.0, 1.0);
    vec3 normal = normalize(normalGeometric.xyz * 2.0 - 1.0);
    float geometricLightDot = clamp(normalGeometric.a, 0.0, 1.0);
    vec3 position = positionMetallic.xyz;

    vec3 albedo = colorRoughness.rgb;

    if (positionMetallic.a < 0.0) {
        fragColor = vec4(SampleSkybox(GetSkyboxDirection(vTexCoord)), 1.0);
        return;
    }

    vec3 viewDir = normalize(uLighting.cameraPosition.xyz - position);
    vec3 lightDir = normalize(-uLighting.lightDirection.xyz);

    vec4 shadowPosition = uLighting.lightViewProjection * vec4(position, 1.0);
    vec3 shadowCoord = shadowPosition.xyz / shadowPosition.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

    float observableArea = GetObservableArea(normal);

    float shadow = GetShadowValue(shadowCoord, normal, lightDir);
    float shadowVisibility = mix(0.12, 1.0, shadow);
    vec3 directLight = EvaluateDirectionalBRDF(albedo, metallic, roughness, normal, viewDir, lightDir, observableArea, shadowVisibility);
    vec3 pointLight = EvaluatePointLights(albedo, metallic, roughness, normal, viewDir, position);
    vec3 ambientLight = EvaluateSkyboxAmbient(albedo, metallic, roughness, normal, viewDir);
    vec3 color = ambientLight + directLight + pointLight;

    fragColor = vec4(color * 8.0f, 1.0);
}
