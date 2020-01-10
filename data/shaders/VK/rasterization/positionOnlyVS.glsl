#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

layout (set=1,binding=0) buffer positions 
{
	vec4 p[];
};

void VS()
{
	vec4 position = p[gl_VertexIndex];
	gl_Position = cameraBuffer.MVP * position;
}