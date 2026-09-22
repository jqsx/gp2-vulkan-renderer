#version 450

layout (location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec3 vWorldPosition;
layout(location = 4) in vec4 vTangent;

layout(location = 0) out vec4 outColorRoughness;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

layout(set = 0, binding = 1) uniform sampler2D uTexture[3];
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightDirection;
} uInput;

layout(push_constant) uniform PushData {
    uint textureIndex;
    float roughness;
} pc;

mat3 GetTangentMatrix(vec3 tangent, vec3 normal)
{
    vec3 binormal = cross(normal, tangent);
    return mat3(tangent, binormal, normal);
}

void main() {
    vec4 textureColor = texture(uTexture[0], vTexCoord);
    if (textureColor.a < 0.5)
        discard;

    vec3 geometricNormal = vNormal;
    vec3 tangent = vTangent.xyz;
    mat3 tangentToWorld = GetTangentMatrix(tangent, geometricNormal);

    vec3 tangentNormal = texture(uTexture[1], vTexCoord).xyz * 2.0 - 1.0;
    vec3 normal = tangentToWorld * tangentNormal;
    float roughness = clamp(texture(uTexture[2], vTexCoord).g * pc.roughness, 0.0, 1.0);
    float metallic = clamp(texture(uTexture[2], vTexCoord).b, 0.0, 1.0);
    float geometricLightDot = max(dot(geometricNormal, normalize(-uInput.lightDirection.xyz)), 0.0);

    outColorRoughness = vec4(textureColor.xyz, roughness);
    outNormal = vec4(normal * 0.5 + 0.5, geometricLightDot);
    outPosition = vec4(vWorldPosition, metallic);
}
