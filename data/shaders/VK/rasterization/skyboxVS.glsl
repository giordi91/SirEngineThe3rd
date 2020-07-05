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

layout(location =1) out vec4 worldPos;
void VS()
{
	vec4 position = p[gl_VertexIndex];
	vec4 offsetPos = position + cameraBuffer.position;
	offsetPos.w = 1.0;
	gl_Position = cameraBuffer.MVP * offsetPos;
	worldPos = position;
}