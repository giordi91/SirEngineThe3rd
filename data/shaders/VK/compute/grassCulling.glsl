#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 

layout (set=3,binding=0) buffer inData 
{
	vec4 value[];
};
layout (set=3,binding=1) buffer outData 
{
	int outValue[];
};

layout (set=3,binding=2) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 


vec3 createCube( uint vertexID)
{
	uint b = 1 << vertexID;
	float x = float((0x287a & b) != 0);
	float y = float((0x02af & b) != 0);
	float z = float((0x31e3 & b) != 0);
	return vec3(x,y,z);
}


layout (local_size_x = 16,local_size_y =1) in;
void CS()
{	
	 vec3 aabbScale = vec3(grassConfig.tileSize, 
						   grassConfig.height + grassConfig.heightRandom,
						   grassConfig.tileSize);
	 vec3 tilePos = value[gl_GlobalInvocationID.x].xyz;

	 int vote = 0;


	for(int v=0; v < 8; ++v)
	{
		vec3 pos = createCube(v) * aabbScale;
		vec3 wp= tilePos + pos;
		int planeVote = 1;
		for (uint p = 0; p < 6; ++p) {
			vec3 pNormal = frameData.m_mainCamera.frustum[p].normal.xyz;
			vec3 pPos = frameData.m_mainCamera.frustum[p].position.xyz;
			//normals is pointing out the volume, so if the dot product is positive
			//means the point is out of the bounding box
			float dist = dot(pNormal, wp.xyz - pPos);
			//if the point is out of the planes it means is not contained with the bounding box
			//to be inside needs to be inside all the planes
			//so as long distance is negative (point inside) we keep anding a 1, keeping the 
			//value alive, if only one point is out, then we and a 0, setting the value to zero
			//forever
			planeVote = planeVote & ((dist > 0.0f) ? 0 : 1);
		}
		//if the point survived then we or it in, since we just need one point surviving 
		//to make the cull fail
		vote = vote | planeVote;
	}
	outValue[gl_GlobalInvocationID.x] = vote;
/*
[unroll(8)]
for (uint v = 0; v < 8; ++v) {
  int planeVote = 1;
  float4 wp = mul(mat, aabb[v]);
  [unroll(6)]
  for (uint p = 0; p < 6; ++p) {
	float3 pNormal = fplanes[p].norm;
	float dist = dot(pNormal, wp.xyz) + fplanes[p].dist;
	planeVote = planeVote & ((dist < 0.0f) ? 0 : 1);
  }
vote = vote | planeVote;
}
voted[id.x] = vote;
*/




	//float val =value[gl_GlobalInvocationID.x].x; 
	//outValue[gl_GlobalInvocationID.x] =(int)(val + 1);
}
