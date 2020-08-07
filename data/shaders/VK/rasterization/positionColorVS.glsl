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

layout (location = 1) out vec4 outColor;

void VS()
{
	vec4 position = pc[gl_VertexIndex].p;
	outColor = pc[gl_VertexIndex].c;
	gl_Position = cameraBuffer.MVP * position;
}