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


vec3 createCube( uint vertexID)
{
	uint b = 1 << vertexID;
	float x = float((0x287a & b) != 0);
	float y = float((0x02af & b) != 0);
	float z = float((0x31e3 & b) != 0);
	return vec3(x,y,z);
}


layout(location =1) out vec4 worldPos;
void VS()
{
	//the cube is in the range 0-1, we want it in the range -0.5 0.5 around the origin
	//so we translate it back to compensate for that
	vec4 position  = vec4((createCube(gl_VertexIndex) - vec3(0.5,0.5,0.5)),1.0);
	vec4 offsetPos = position+ cameraBuffer.position;
	offsetPos.w = 1.0;
	gl_Position = cameraBuffer.MVP * offsetPos;
	worldPos = position;

}