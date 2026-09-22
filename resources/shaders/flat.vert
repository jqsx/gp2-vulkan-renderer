#version 450

layout(location = 0) in vec3 aPosition; // a for attrib
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec4 aTangent;

layout(location = 0) out vec3 vColor; // v for varrying GLSL 1.0 habbit from canvas.getContext("gl")
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out vec3 vNormal;
layout(location = 3) out vec3 vWorldPosition;
layout(location = 4) out vec4 vTangent;

// u for uniform

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightDirection;
} uInput;

void main() {
    vec4 worldPosition = uInput.model * vec4(aPosition, 1.0);
    gl_Position = uInput.proj * uInput.view * worldPosition;
    vColor = aColor;
    vTexCoord = aTexCoord;
    vNormal = mat3(uInput.model) * aNormal;
    vWorldPosition = worldPosition.xyz;
    vTangent = vec4(mat3(uInput.model) * aTangent.xyz, aTangent.w);
}
