#version 450

#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_vote: enable
#extension GL_KHR_shader_subgroup_arithmetic: require 

#include "../common/structures.glsl"

/*
layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 
*/

layout (set=3,binding=0) buffer inVote
{
	int vote[];
};
//holds the indices of the tiles that survived
layout (set=3,binding=1) buffer outData 
{
	ivec2 outValue[];
};

layout (set=3,binding=2) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 
layout (set=3,binding=3) buffer tileIndices 
{
	int tileIds[];
};

const int SUPPORT_DATA_ATOMIC_OFFSET = 8;
const int SUPPORT_DATA_OFFSET = 16;

layout (local_size_x = 64,local_size_y =1) in;
void CS()
{	
  //if the value is not in range we reduce it to 0 so won't affect the scan and 
  //we don't need a defensive branch, we allocate enough memory anyway to the multiple
  //of the group
  int totalTiles = grassConfig.tilesPerSide*grassConfig.tilesPerSide;
  int isInRange = int(gl_GlobalInvocationID.x < totalTiles);
  int v = vote[gl_GlobalInvocationID.x] * isInRange;
  //need to fix for 64-32 wave size 
  int scan = subgroupInclusiveAdd(v);

  //perform the atomic increase
  int offset =0;
  if(gl_LocalInvocationID.x ==63){
	offset = atomicAdd(outValue[SUPPORT_DATA_ATOMIC_OFFSET].x,scan);
  }
  offset = subgroupBroadcast(offset,63);

  if(v != 0 )
  {
	outValue[offset + scan + SUPPORT_DATA_OFFSET-1] = 
	ivec2(int(gl_GlobalInvocationID.x),int(tileIds[gl_GlobalInvocationID.x]));
  }
}
