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

layout (set=1,binding=1) uniform g_settings 
{
	vec4 color;
    float pointSize;
}; 

const vec3 offsets[6] =
{
    { -1.0f, -1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f }
};

void VS()
{
    vec3 offset = offsets[gl_VertexIndex % 6] * pointSize;

    vec3 up = { 0, 1, 0 };
    vec3 view = cameraBuffer.cameraViewDir.xyz;

    offset = ( mat3x3(cameraBuffer.ViewMatrix)*offset) + p[gl_VertexIndex/6].xyz;

	// Transform to homogeneous clip space.
    gl_Position = vec4(offset, 1.0);
}