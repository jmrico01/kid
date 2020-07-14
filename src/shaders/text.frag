#version 450

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D textureSampler;

void main()
{
    float opacity = texture(textureSampler, inUv).r;
    float gamma = 2.2;
    float opacityGammaCorrected = pow(opacity, gamma);

    outColor = vec4(1.0, 1.0, 1.0, opacityGammaCorrected);
}
