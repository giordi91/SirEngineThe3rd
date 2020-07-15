#version 450

#extension GL_GOOGLE_include_directive: require
#include "../common/structures.glsl"

layout (set=3,binding = 3) uniform texture2D colorTexture;
//pass data, lights
layout (set=2,binding=0) uniform LightData 
{
	DirectionalLightData lightData;
}; 
layout (set=1,binding = 0) uniform sampler[7] colorSampler;

layout(location=0) out vec4 outputColor;

layout (location = 0) in vec3 normals;
layout (location = 1) in vec2 inUV;

void PS()
{
   vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   vec3 l  = -(normalize(vec3(-1,-1,-1)));
   float d = dot(l,normals);
   outputColor = vec4(d,d,d,1.0);
   //outputColor = texture (sampler2D (colorTexture, colorSampler[2]), uv);
   //outputColor = color;
}
