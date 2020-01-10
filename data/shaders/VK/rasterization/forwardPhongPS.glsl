#version 450


layout (set=1,binding = 3) uniform texture2D colorTexture;
layout (set=2,binding = 0) uniform sampler[7] colorSampler;

layout(location=0) out vec4 outputColor;

layout(location=0) in vec4 color;
layout (location = 1) in vec2 inUV;

void PS()
{
   vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   outputColor = texture (sampler2D (colorTexture, colorSampler[2]), uv);
   //outputColor = color;
}
