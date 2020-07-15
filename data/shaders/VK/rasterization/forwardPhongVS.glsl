#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"


//struct Vertex {
//  float vx, vy, vz;
//  uint8_t nx, ny, nz, nw;
//  float tu, tv;
//};
//
//layout (binding=0) buffer Vertices
//{ 
//	Vertex vertices[];
//};
layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

//struct normal{uint8_t nx, ny, nz, nw;};

layout (set=3,binding=0) buffer vertices
{
	vec4 p[];
};
layout (set=3,binding=1) buffer normals 
{
	vec4 n[];
};
layout (set=3,binding=2) buffer uvs 
{
	vec2 uv[];
};


layout(location =0) out vec3 outNormal;
layout(location =1) out vec2 outUV;
void VS()
{
	//this can't be used becuase seems like is making a copy and the 
	//driver isn't happy about it and violates vulkan 1.1 specs, need to
	//extract directly from the struct blearg
	//Vertex v = vertices[gl_VertexIndex];

	//vec3 position = vec3(vertices[gl_VertexIndex].vx,vertices[gl_VertexIndex].vy,vertices[gl_VertexIndex].vz);
	//vec3 normal = vec3(int(vertices[gl_VertexIndex].nx),int(vertices[gl_VertexIndex].ny),int(vertices[gl_VertexIndex].nz)) /127.0 -1.0f;
	//uv= vec2(vertices[gl_VertexIndex].tu,vertices[gl_VertexIndex].tv);

	vec4 position = p[gl_VertexIndex];
	//vec3 position = vec3(p[gl_VertexIndex].x,p[gl_VertexIndex].y,p[gl_VertexIndex].z);
	outNormal= n[gl_VertexIndex].xyz;
	outUV= uv[gl_VertexIndex];

	gl_Position = cameraBuffer.MVP * position;
}

