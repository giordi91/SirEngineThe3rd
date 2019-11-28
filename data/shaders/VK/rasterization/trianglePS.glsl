#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout (set = 0, binding = 1) uniform sampler2D samplerColorMap;
layout (set=0,binding = 1) uniform texture2D colorTexture;
layout (set=0,binding = 2) uniform sampler colorSampler;

layout(location=0) out vec4 outputColor;

layout(location=0) in vec4 color;
layout (location = 1) in vec2 inUV;

void PS()
{
   outputColor = texture (sampler2D (colorTexture, colorSampler), inUV);
	//outputColor = texture(samplerColorMap, inUV);
}
