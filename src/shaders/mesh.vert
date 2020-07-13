#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUv;
layout(location = 4) in float inLightmapWeight;

// TODO might be worth looking into flat-shading support, e.g. don't interpolate normals
// Might even be possible to save some duplicate vertex info and bring back indexing
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outUv;
layout(location = 3) out float outLightmapWeight;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    outNormal = inNormal;
    outColor = inColor;
    outUv = inUv;
    outLightmapWeight = inLightmapWeight;

	vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = pos;
}
