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

const float width = 0.2;
const float height = 0.4;

const vec4 offsets[3] = {
    vec4(1.0f, 0.0f, 0.0f, width), 
    vec4(0.0f, 1.0f, 0.0f, height), 
    vec4(-1.0f, 0.0f, 0.0f, width), 
};


layout(location =1) out vec2 outUV;
void VS()
{
    uint vid = gl_VertexIndex/3;
	uint localId = gl_VertexIndex%3;
	vec4 position = p[vid];
	float width =0.2;
	float height =width*2;
	vec4 offset = offsets[localId];
	offset*=offset.w;
	offset.w=0.0f;

	outUV= vec2(0,0);

	gl_Position = cameraBuffer.MVP * (position+offset);
}

