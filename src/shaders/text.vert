#version 450

// Vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;

// Instance attributes
layout(location = 2) in vec3 inOrigin;
layout(location = 3) in vec2 inSize;
layout(location = 4) in vec4 inUvInfo;
layout(location = 5) in vec4 inColor;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;

void main() {
    outUv = inUv * inUvInfo.zw + inUvInfo.xy;
    outColor = inColor;

    gl_Position = vec4(vec3(inPosition * inSize, 0.0) + inOrigin, 1.0);
}
