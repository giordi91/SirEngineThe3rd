#version 450

#extension GL_KHR_vulkan_glsl: enable 
#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"


//const vec4 arrBasePos[6] = {
//    vec4(-1.0f, 1.0f, 0.0f, 0.0f), 
//	vec4(1.0f, 1.0f, 1.0f, 0.0f),
//    vec4(-1.0f, -1.0f, 0.0f, 1.0f), 
//	vec4(1.0f, -1.0f, 1.0f, 1.0f),
//	vec4(-1.0f, -1.0f, 0.0f, 1.0f),
//	vec4(1.0f, 1.0f, 1.0f, 0.0f)
//	};

const vec4 arrBasePos[6] = {
    vec4(-1.0f, -1.0f, 0.0f, 1.0f), 
	vec4(1.0f, 1.0f, 1.0f, 0.0f),
    vec4(-1.0f, 1.0f, 0.0f, 0.0f), 
	vec4(1.0f, 1.0f, 1.0f, 0.0f),
	vec4(-1.0f, -1.0f, 0.0f, 1.0f),
	vec4(1.0f, -1.0f, 1.0f, 1.0f)
	};

layout(location =0) out vec2 outUV;
void VS()
{
	vec4 position = arrBasePos[gl_VertexIndex];
	gl_Position = vec4(position.xy,0.5f,1.0f);
	outUV = position.zw;
}

