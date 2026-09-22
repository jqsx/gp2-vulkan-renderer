#version 450

layout(location = 0) in vec3 vWorldPosition;

layout(push_constant) uniform PointShadowMatrices {
    mat4 model;
    mat4 lightViewProj;
    vec4 lightPositionRadius;
} uShadow;

void main() {
    float distanceToLight = length(vWorldPosition - uShadow.lightPositionRadius.xyz);
    gl_FragDepth = clamp(distanceToLight / uShadow.lightPositionRadius.w, 0.0, 1.0);
}
