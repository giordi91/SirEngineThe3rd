#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require


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

struct Normal{uint8_t nx, ny, nz, nw;};

layout (binding=1) buffer vertices
{
	vec3 p[];
};
layout (binding=2) buffer normals 
{
	Normal n[];
};
layout (binding=3) buffer uvs 
{
	vec2 uv[];
};

layout (binding=0) uniform CameraBuffer{
  mat4 MVP;
  mat4 ViewMatrix;
  mat4 VPinverse;
  vec4 perspectiveValues;
  vec4 position;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;

} cameraBuffer; 

layout(location =0) out vec4 color;
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

	//vec3 position = p[gl_VertexIndex];
	vec3 position = vec3(p[gl_VertexIndex].x,p[gl_VertexIndex].y,p[gl_VertexIndex].z);
	vec3 normal = vec3(int(n[gl_VertexIndex].nx),int(n[gl_VertexIndex].ny),int(n[gl_VertexIndex].nz));
	outUV= uv[gl_VertexIndex];

	gl_Position = cameraBuffer.MVP * vec4(position + vec3(0,0,0.5),1.0f);
	//gl_Position = vec4(position + vec3(0,0,0.5),1.0f);
	color = vec4(normal*0.5f + vec3(0.5f),1.0f);
}

