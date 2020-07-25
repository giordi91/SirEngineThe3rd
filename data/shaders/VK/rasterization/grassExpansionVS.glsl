#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

layout (set=3,binding=0) buffer vertices
{
	vec4 p[];
};

const float width = 0.2;
const float height = 0.4;

    //vec4(1.0f, 0.0f, 0.0f, width), 
    //vec4(0.0f, 1.0f, 0.0f, height), 
    //vec4(-1.0f, 0.0f, 0.0f, width), 
const vec3 offsets[15] = {
    vec3(-0.5f, 0.0f, 0.0f), 
    vec3(0.5f, 0.0f, 0.0f), 
    vec3(-0.5f, 0.33333f, 0.0f), 

    vec3(0.5f, 0.0f, 0.0f), 
    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(-0.5f, 0.33333f, 0.0f), 

    vec3(-0.5f, 0.33333f, 0.0f), 
    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(-0.5f, 0.66666f, 0.0f), 

    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(0.5f, 0.66666, 0.0f), 
    vec3(-0.5f, 0.66666, 0.0f), 

    vec3(-0.5f, 0.66666, 0.0f), 
    vec3(0.5f, 0.66666, 0.0f), 
    vec3(0.0f, 1.0f, 0.0f) 
};

const vec2 bladeUVs[15] = {
    vec2(0.0f, 0.0f), 
    vec2(1.0f, 0.0f), 
    vec2(0.0f, 0.333333f), 

    vec2(1.0f, 0.0f), 
    vec2(1.0f, 0.333333f), 
    vec2(0.0f, 0.333333f), 

    vec2(0.0f, 0.33f), 
    vec2(1.0f, 0.33), 
    vec2(0.0f, 0.66666f), 

    vec2(1.0f, 0.33f), 
    vec2(1.0f, 0.666666), 
    vec2(0.0f, 0.666666), 

    vec2(0.0f, 0.6666f), 
    vec2(1.0f, 0.6666f), 
    vec2(0.5f, 1.0), 

};

const vec3 taper = vec3(1.0f, 0.6f ,1.0f);

layout(location =1) out vec2 outUV;
void VS()
{
    uint vid = gl_VertexIndex/15;
	uint localId = gl_VertexIndex%15;
	vec4 position = p[vid];
	vec4 offset = vec4(offsets[localId],0.0f);

    //vec2 uv = uvs[vid];
    vec2 uv = bladeUVs[localId];
    
    float a = taper.x * float(uv.y < 0.33f);
    float b = taper.y * float(int (uv.y <= 0.66) & int(uv.y >= 0.33));
	float c = taper.z * float(uv.y > 0.6666666);

    float taperValue  = a+ b + c;

    //let us compute the triangle we are in
    uint triangleId = localId/6;
    float  localWidth =  width * taperValue;

    offset.x *=localWidth;
    offset.y *=height;
    offset.z *=localWidth;

	position+= offset;


	outUV= uv;

	gl_Position = cameraBuffer.MVP * (position);
}

