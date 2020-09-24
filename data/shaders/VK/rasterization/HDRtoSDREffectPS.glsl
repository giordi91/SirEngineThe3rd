#version 450

layout (set=3,binding = 0) uniform texture2D colorTexture;
layout(set = 1, binding = 0) uniform sampler gsamPointWrap;
layout(set = 1, binding = 1) uniform sampler gsamPointClamp;
layout(set = 1, binding = 2) uniform sampler gsamLinearWrap;
layout(set = 1, binding = 3) uniform sampler gsamLinearClamp;
layout(set = 1, binding = 4) uniform sampler gsamAnisotropicWrap;
layout(set = 1, binding = 5) uniform sampler gsamAnisotropicClamp;
layout(location=0) out vec4 outputColor;

layout (location = 0) in vec2 inUV;

void PS()
{
   outputColor = texture (sampler2D (colorTexture, gsamLinearWrap), inUV);
   //outputColor = color;
   //outputColor =vec4(uv,0,1);
   //outputColor =vec4(1,0,0,1);
}
