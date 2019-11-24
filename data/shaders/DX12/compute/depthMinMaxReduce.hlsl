#include "../common/structures.hlsl"

Texture2D<float> Input : register(t0);
ConstantBuffer<CameraBuffer> g_camera : register(b0);
RWStructuredBuffer<ReducedDepth> reducedDepth : register(u0);

#if AMD
static const int GROUP_SIZE_X = 64;
static const int GROUP_SIZE_Y = 4;
#else
static const int GROUP_SIZE_X = 32;
static const int GROUP_SIZE_Y = 8;
#endif
groupshared float sharedData[GROUP_SIZE_Y*2];

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void CS(uint3 id : SV_DispatchThreadID, int3 localId : SV_GroupThreadID)
{
    if (id.x < g_camera.screenWidth && id.y < g_camera.screenHeight)
    {
        float depth = Input[id.xy];
        float minDepth = WaveActiveMin(depth);
        float maxDepth = WaveActiveMax(depth);

        //this means only the y values will go in
        if (localId.x == 0)
        {
            //let us store both min and max
            sharedData[localId.y] = minDepth;
            sharedData[localId.y + GROUP_SIZE_Y] = maxDepth;
        }
        GroupMemoryBarrierWithGroupSync();

        bool shouldLoad = localId.x < GROUP_SIZE_Y && localId.y == 0;
            //now we load the data back in
        float sharedMinDepth = shouldLoad ? sharedData[localId.x] : 1.0f;
        float sharedMaxDepth = shouldLoad ? sharedData[localId.x] : 0.0f;

        //reducing the final wave
        float finalMinDepth = WaveActiveMin(sharedMinDepth);
        float finalMaxDepth = WaveActiveMax(sharedMaxDepth);

        //now that the reduction is done we have 4 waves worth
        //of data to be stored in shared memory
        uint minDepthInt = asuint(finalMinDepth);
        uint maxDepthInt = asuint(finalMaxDepth);


        bool shouldWrite = (localId.x == 0) & (localId.y == 0);
        if (shouldWrite)
        {
            InterlockedMin(reducedDepth[0].minDepth, minDepthInt);
            InterlockedMax(reducedDepth[0].maxDepth, maxDepthInt);
        }
    }
}
