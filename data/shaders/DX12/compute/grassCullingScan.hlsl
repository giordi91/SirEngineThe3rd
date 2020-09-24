#include "../common/structures.hlsl"

struct GrassCullingResult
{
    int tileIndex;
    uint16_t tilePointsId;
    uint16_t LOD;
};

ConstantBuffer<GrassConfig> grassConfig: register(b2, space3);

ConstantBuffer<FrameData> frameData: register(b0, space0);

RWByteAddressBuffer outData: register(u1, space3);
ByteAddressBuffer inTileIndices: register(t3, space3);
ByteAddressBuffer inTileData: register(t0, space3);


static const int SUPPORT_DATA_ATOMIC_OFFSET0 = 8;
static const int SUPPORT_DATA_ATOMIC_OFFSET1 = 16;
static const int SUPPORT_DATA_ATOMIC_OFFSET2 = 24;
static const int SUPPORT_DATA_ATOMIC_OFFSET3 = 32;
static const int SUPPORT_DATA_OFFSET = 40;

groupshared int accum;
groupshared int offset;

float3 createCube(uint vertexID)
{
    uint b = uint(1 << int(vertexID));
    float x = float((10362u & b) != 0u);
    float y = float((687u & b) != 0u);
    float z = float((12771u & b) != 0u);
    return float3(x, y, z);
}

int isTileVisiable(float3 tilePos)
{
    float3 aabbScale = float3(grassConfig.tileSize, grassConfig.height + grassConfig.heightRandom, grassConfig.tileSize);
    int vote = 0;
    for (int v = 0; v < 8; v++)
    {
        uint param = uint(v);
        float3 pos = createCube(param) * aabbScale;
        float3 wp = tilePos + pos;
        int planeVote = 1;
        for (uint p = 0u; p < 6u; p++)
        {
            float3 pNormal = frameData.m_mainCamera.frustum[p].normal.xyz;
            float3 pPos = frameData.m_mainCamera.frustum[p].position.xyz;
			//normals is pointing out the volume, so if the dot product is positive
			//means the point is out of the bounding box
            float dist = dot(pNormal, wp - pPos);
			//if the point is out of the planes it means is not contained with the bounding box
			//to be inside needs to be inside all the planes
			//so as long distance is negative (point inside) we keep anding a 1, keeping the 
			//value alive, if only one point is out, then we and a 0, setting the value to zero
			//forever
            planeVote &= ((dist > 0.0f) ? 0 : 1);
        }
		//if the point survived then we or it in, since we just need one point surviving 
		//to make the cull fail
        vote |= planeVote;
    }
    return vote;
}

uint16_t getLOD(float3 tilePos)
{
    float3 aabbScale = float3(grassConfig.tileSize, grassConfig.height + grassConfig.heightRandom, grassConfig.tileSize);
    float3 center = tilePos + (aabbScale * 0.5f);
    float dist = length(center - frameData.m_mainCamera.position.xyz);
	//find which lod slot are we in
    int lod1 = int(dist > grassConfig.lodThresholds.x) & int(dist <= grassConfig.lodThresholds.y);
    int lod2 = int(dist > grassConfig.lodThresholds.y) & int(dist <= grassConfig.lodThresholds.z);
    int lod3 = int(dist > grassConfig.lodThresholds.z);
    int finalLod = (lod1 + (lod2 * 2)) + (lod3 * 3);
    return (int16_t(finalLod));
}

//    uint3 gl_LocalInvocationID : lid;
//    uint3 gl_GlobalInvocationID : gid;
void scanLod(int cullVote, int lod, int wantedLod, int lodAtomicIndex, uint gid, uint lid)
{
    int vote = (lod == wantedLod) ? cullVote : 0;
	//if the value is not in range we reduce it to 0 so won't affect the scan and 
	//we don't need a defensive branch, we allocate enough memory anyway to the multiple
	//of the group
    int totalTiles = grassConfig.tilesPerSide * grassConfig.tilesPerSide;
    int isInRange = int(gid < uint(totalTiles));
    vote *= isInRange;
    int scan = WavePrefixSum(vote) + vote;
    uint wavesPerBlock = 64u / WaveGetLaneCount();
    if (lid == 0u)
    {
        accum = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    for (int i = 1; uint(i) < wavesPerBlock; i++)
    {
        uint waveDelimiter = (WaveGetLaneCount() * uint(i)) - 1u;
        if (lid == waveDelimiter)
        {
			//copy to local memory
            accum += scan;
        }
        GroupMemoryBarrierWithGroupSync();
        if (lid > waveDelimiter)
        {
            scan += accum;
        }
    }
    //perform the atomic increase
    if (lid == 63u)
    {
        outData.InterlockedAdd(lodAtomicIndex * 8 + 0, scan, offset);
    }
    GroupMemoryBarrierWithGroupSync();
	//if we did not have to support multiple wave len we could 
	//use a wave broadcast but won't work with waves smaller than the
	//block size
    int actualOffset = offset;
    if (vote != 0)
    {
        GrassCullingResult res;
        res.tileIndex = int(gid);
        res.tilePointsId = (int16_t(inTileIndices.Load<int>(gid * 4 + 0)));
        res.LOD = (int16_t(lod));
        int storeOffset= (((actualOffset + scan) + 40) + (lod * totalTiles)) - 1;
        outData.Store<int>(storeOffset* 8 + 0, res.tileIndex);
        outData.Store<uint16_t>(storeOffset* 8 + 4, res.tilePointsId);
        outData.Store<uint16_t>(storeOffset* 8 + 6, res.LOD);
    }
}

[numthreads(64, 1, 1)]
void CS(uint3 gid: SV_DispatchThreadID, uint3 lid : SV_GroupThreadID)
{
    float3 tilePos = inTileData.Load<float4>(gid.x * 16 + 0).xyz;
    float3 param = tilePos;
    int cullVote = isTileVisiable(param);
    uint16_t LOD = getLOD(tilePos);
    scanLod(cullVote, LOD, 0, SUPPORT_DATA_ATOMIC_OFFSET0,gid.x,lid.x);
    GroupMemoryBarrierWithGroupSync();
    scanLod(cullVote, LOD, 1, SUPPORT_DATA_ATOMIC_OFFSET1,gid.x,lid.x);
    GroupMemoryBarrierWithGroupSync();
    scanLod(cullVote, LOD, 2, SUPPORT_DATA_ATOMIC_OFFSET2,gid.x,lid.x);
    GroupMemoryBarrierWithGroupSync();
    scanLod(cullVote, LOD, 3, SUPPORT_DATA_ATOMIC_OFFSET3,gid.x,lid.x);
    GroupMemoryBarrierWithGroupSync();
}
