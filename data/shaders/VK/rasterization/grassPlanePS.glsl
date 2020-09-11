#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout(location=0) out vec4 outputColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 worldPos;

layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 


//pass data, lights
layout (set=2,binding=0) uniform LightData 
{
	DirectionalLightData lightData;
}; 
layout (set=2,binding = 1) uniform textureCube skyboxIrradianceTexture;
layout (set=2,binding = 2) uniform textureCube skyboxRadianceTexture;
layout (set=2,binding = 3) uniform texture2D brdfTexture;

layout (set=3,binding=3) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 

layout (set=3,binding = 4) uniform texture2D albedoTex;
layout (set=1,binding = 0) uniform sampler[7] colorSampler;



void PS()
{
   vec3 albedo = texture (sampler2D (albedoTex, colorSampler[2]), inUV*5.0).xyz;
   outputColor = vec4(albedo,1.0f);

}
