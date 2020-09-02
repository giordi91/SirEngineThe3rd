#version 450

#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_vote: enable
#extension GL_KHR_shader_subgroup_arithmetic: require 
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types :require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require

#include "../common/structures.glsl"
#include "../common/utility.glsl"

struct GrassCullingResult
{
    int tileIndex; 
	uint16_t tilePointsId; 
	uint16_t LOD; 
};

layout (set=0,binding=0) uniform readonly inFrameData 
{
	FrameData frameData;
}; 

layout (set=3,binding=0) buffer readonly inTileData
{
	vec4 inputTilePositions[];
};
//holds the indices of the tiles that survived
layout (set=3,binding=1) buffer outData 
{
	GrassCullingResult outValue[];
};

layout (set=3,binding=2) uniform inConfigData 
{
	GrassConfig grassConfig;
}; 
layout (set=3,binding=3) buffer readonly inTileIndices 
{
	int tileIds[];
};

const int SUPPORT_DATA_ATOMIC_OFFSET = 8;
const int SUPPORT_DATA_OFFSET = 16;


int isTileVisiable(vec3 tilePos)
{
	vec3 aabbScale = vec3(grassConfig.tileSize, 
						   grassConfig.height + grassConfig.heightRandom,
						   grassConfig.tileSize);

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
	return vote;
}


shared int offset;
shared int accum;
const int BLOCK_SIZE = 64;

layout (local_size_x = BLOCK_SIZE ,local_size_y =1) in;
void CS()
{	

  vec3 tilePos = inputTilePositions[gl_GlobalInvocationID.x].xyz;
  int vote = isTileVisiable(tilePos);


  //if the value is not in range we reduce it to 0 so won't affect the scan and 
  //we don't need a defensive branch, we allocate enough memory anyway to the multiple
  //of the group
  int totalTiles = grassConfig.tilesPerSide*grassConfig.tilesPerSide;
  int isInRange = int(gl_GlobalInvocationID.x < totalTiles);
  vote *= isInRange;
  //int v = vote[gl_GlobalInvocationID.x] * isInRange;
  //need to fix for 64-32 wave size 
  int scan = subgroupInclusiveAdd(vote);


  uint wavesPerBlock =  64 / gl_SubgroupSize;

  if(gl_LocalInvocationID.x ==0)
  {
	accum =0;
  }
  memoryBarrierShared();

  for(int i =1; i <wavesPerBlock;++i)
  {
	uint waveDelimiter = (gl_SubgroupSize*i) -1;
	if(gl_LocalInvocationID.x == waveDelimiter)
	{
	  //copy to local memory
	  accum += scan;
	}
    memoryBarrierShared();
  //barrier();
	if(gl_LocalInvocationID.x > waveDelimiter )
	{
		scan += accum;
	}
  }

  //perform the atomic increase
  if(gl_LocalInvocationID.x ==63){
	offset = atomicAdd(outValue[SUPPORT_DATA_ATOMIC_OFFSET].tileIndex ,scan);
  }
  barrier();
  //if we did not have to support multiple wave len we could 
  //use a wave broadcast but won't work with waves smaller than the
  //block size
  int actualOffset = offset;

  if(vote != 0 )
  {
    GrassCullingResult res;
	res.tileIndex = int(gl_GlobalInvocationID.x);
	res.tilePointsId = uint16_t(tileIds[gl_GlobalInvocationID.x]);
	res.LOD = uint16_t(0);

	outValue[actualOffset + scan + SUPPORT_DATA_OFFSET-1] =  res;
  }
}
