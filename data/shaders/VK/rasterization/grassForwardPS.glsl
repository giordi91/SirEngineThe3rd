#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"


const vec3 baseColor = vec3(0.015,0.455,0.026);
const vec3 topColor = vec3(0.357,0.67,0.09);

layout(location=0) out vec4 outputColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (set=3,binding=3) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 


void PS()
{
   //vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   vec4 color= vec4(mix(grassConfig.baseColor,grassConfig.tipColor,inUV.y),1.0f);

   outputColor = color;
}
