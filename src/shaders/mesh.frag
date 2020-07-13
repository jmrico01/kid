#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in float inLightmapWeight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D lightmapSampler;

vec3 lerp(vec3 a, vec3 b, float t)
{
	return a * (1.0 - t) + b * t;
}

void main()
{
	vec3 colorAmbient = vec3(0.0, 0.0, 0.0);
	vec3 colorLightmap = texture(lightmapSampler, inUv).rgb;
    vec3 colorFinal = colorAmbient + lerp(inColor, colorLightmap, inLightmapWeight);

	outColor = vec4(colorFinal, 1.0);
}
