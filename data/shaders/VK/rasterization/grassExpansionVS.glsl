#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

layout (set=1,binding=0) buffer vertices
{
	vec4 p[];
};

layout(location =1) out vec2 outUV;
void VS()
{
	vec4 position = p[gl_VertexIndex];
	outUV= vec2(0,0);

	gl_Position = cameraBuffer.MVP * position;
}

