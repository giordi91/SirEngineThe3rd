#include "../common/structures.hlsl"

Texture2D<float> Input : register( t0 );
ConstantBuffer<CameraBuffer> g_camera: register(b0);
RWStructuredBuffer<ReducedDepth> reducedDepth : register(u0);

[numthreads( 64, 4, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{

    if (id.x < g_camera.screenWidth && id.y < g_camera.screenHeight  )
    {
        float depth = Input[id.xy];
        float minDepth = WaveActiveMin(depth);
        float maxDepth = WaveActiveMax(depth);

        uint minDepthInt = asuint(minDepth);
        uint maxDepthInt = asuint(maxDepth);

        InterlockedMin(reducedDepth[0].minDepth, minDepthInt);
        InterlockedMax(reducedDepth[0].maxDepth, maxDepthInt);
    }
}
