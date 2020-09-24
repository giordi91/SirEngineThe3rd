
#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=3,binding = 0) uniform texture2D colorTexture;
layout (set=3,binding=1) uniform InputData 
{
	GammaToneMappingConfig g_config;
}; 
layout (set=1,binding = 2) uniform sampler gsamLinearWrap ;

layout(location=0) out vec4 outputColor;

layout (location = 0) in vec2 inUV;

void PS()
{
   vec3 color = texture (sampler2D (colorTexture, gsamLinearWrap), inUV).xyz;
	//exposure tone mapping
	vec3 mapped = vec3(1.0f,1.0f,1.0f) -exp(-color* g_config.exposure);
	//gamma
	mapped = pow(mapped, vec3(g_config.gammaInverse,g_config.gammaInverse,g_config.gammaInverse) );
	//mapped = pow(mapped, 1.0f/2.2f);
	outputColor = vec4(mapped,1.0f);
}
