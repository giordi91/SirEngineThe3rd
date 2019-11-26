#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

struct Vertex {
  float vx, vy, vz;
  uint8_t nx, ny, nz, nw;
  float tu, tv;
};

layout (binding=0) buffer Vertices
{ 
	Vertex vertices[];
};

layout(location =0) out vec4 color;
layout(location =1) out vec2 uv;
void VS()
{
	//this can't be used becuase seems like is making a copy and the 
	//driver isn't happy about it and violates vulkan 1.1 specs, need to
	//extract directly from the struct blearg
	//Vertex v = vertices[gl_VertexIndex];
	vec3 position = vec3(vertices[gl_VertexIndex].vx,vertices[gl_VertexIndex].vy,vertices[gl_VertexIndex].vz);
	vec3 normal = vec3(int(vertices[gl_VertexIndex].nx),int(vertices[gl_VertexIndex].ny),int(vertices[gl_VertexIndex].nz)) /127.0 -1.0f;
	uv= vec2(vertices[gl_VertexIndex].tu,vertices[gl_VertexIndex].tv);
	gl_Position = vec4(position + vec3(0,0,0.5),1.0f);
	color = vec4(normal*0.5f + vec3(0.5f),1.0f);
}