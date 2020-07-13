#version 450

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D textureSampler;

void main()
{
    outColor = texture(textureSampler, inUv);
}
