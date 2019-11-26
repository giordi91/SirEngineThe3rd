#version 450

layout (set = 0, binding = 1) uniform sampler2D samplerColorMap;

layout(location=0) out vec4 outputColor;

layout(location=0) in vec4 color;
layout (location = 1) in vec2 inUV;

void main()
{
	outputColor = texture(samplerColorMap, inUV);
}
