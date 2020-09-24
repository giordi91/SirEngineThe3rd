#version 450

layout (set=3,binding = 1) uniform textureCube skyTexture;
layout(set = 1, binding = 0) uniform sampler gsamPointWrap;
layout(set = 1, binding = 1) uniform sampler gsamPointClamp;
layout(set = 1, binding = 2) uniform sampler gsamLinearWrap;
layout(set = 1, binding = 3) uniform sampler gsamLinearClamp;
layout(set = 1, binding = 4) uniform sampler gsamAnisotropicWrap;
layout(set = 1, binding = 5) uniform sampler gsamAnisotropicClamp;
layout(location=0) out vec4 outputColor;

layout (location = 1) in vec4 worldPos;

void PS()
{
   outputColor = texture(samplerCube (skyTexture, gsamLinearWrap), worldPos.xyz);
   //outputColor = vec4(1,0,0,1);
}
