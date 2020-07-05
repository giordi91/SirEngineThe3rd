#version 450

layout (set=1,binding = 1) uniform textureCube skyTexture;
layout (set=2,binding = 0) uniform sampler[7] colorSampler;

layout(location=0) out vec4 outputColor;

layout (location = 1) in vec4 worldPos;

void PS()
{
   outputColor = texture(samplerCube (skyTexture, colorSampler[2]), worldPos.xyz);
   //outputColor = vec4(1,0,0,1);
}
