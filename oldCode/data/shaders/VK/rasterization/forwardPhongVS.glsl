#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"


layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 


layout (set=3,binding=0) buffer readonly vertices
{
	vec4 p[];
};
layout (set=3,binding=1) buffer readonly normals 
{
	vec4 n[];
};
layout (set=3,binding=2) buffer readonly uvs 
{
	vec2 uv[];
};
layout (set=3,binding=3) buffer readonly tangents 
{
	vec4 tans[];
};


layout(location =0) smooth out vec3 outNormal;
layout(location =1) out vec3 outTan;
layout(location =2) out vec2 outUV;
layout(location =3) out vec3 worldPos;
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
	outTan= tans[gl_VertexIndex].xyz;
	outUV= uv[gl_VertexIndex];

	gl_Position = frameData.m_activeCamera.MVP * position;
	worldPos = position.xyz;
}
