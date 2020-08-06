#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

struct PointColor
{
	vec4 p;
	vec4 c;
};

layout (set=3,binding=0) buffer positions 
{
	PointColor pc[];
};

void VS()
{
	vec4 position = pc[gl_VertexIndex].p;
	if(gl_VertexIndex == 0)
	{
	position = vec4(0,0,0,1);
	}
	if(gl_VertexIndex == 1)
	{
	position = vec4(0,10,0,1);
	}

	if(gl_VertexIndex == 2)
	{
	position = vec4(0,10,0,1);
	}
	if(gl_VertexIndex == 3)
	{
	position = vec4(0,10,10,1);
	}

	if(gl_VertexIndex == 4)
	{
	position = vec4(0,10,10,1);
	}
	if(gl_VertexIndex == 5)
	{
	position = vec4(10,10,10,1);
	}
	gl_Position = cameraBuffer.MVP * position;
}