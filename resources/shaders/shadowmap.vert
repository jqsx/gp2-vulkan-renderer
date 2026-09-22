#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec4 aTangent;

layout(location = 4) out vec4 vTangent;

layout(push_constant) uniform ShadowMatrices {
    mat4 model;
    mat4 lightViewProj;
} uShadow;

void main() {
    vTangent = aTangent;
    gl_Position = uShadow.lightViewProj * uShadow.model * vec4(aPosition, 1.0);
}
