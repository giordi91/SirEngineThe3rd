#version 450

layout (set=1,binding = 1) uniform texture2D colorTexture;
layout (set=2,binding = 0) uniform sampler[7] colorSampler;

layout(location=0) out vec4 outputColor;

layout (location = 0) in vec2 inUV;

void PS()
{
   outputColor = texture (sampler2D (colorTexture, colorSampler[2]), inUV);
   //outputColor = color;
   //outputColor =vec4(uv,0,1);
   //outputColor =vec4(1,0,0,1);
}
