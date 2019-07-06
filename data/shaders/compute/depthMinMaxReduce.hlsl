#include "../common/structures.hlsl"

Texture2D<float> Input : register( t0 );
ConstantBuffer<TextureConfig> g_textureConfig: register(b0);
RWStructuredBuffer<ReducedDepth> reducedDepth : register(u0);

[numthreads( 32, 4, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{

    //if (id.x < g_textureConfig.width && id.y < g_textureConfig.height)
    if (id.x < 1200 && id.y < 600)
    {
        float depth = Input[id.xy] + g_textureConfig.height / 4;
        float minDepth = WaveActiveMin(depth);
        float maxDepth = WaveActiveMax(depth);

        uint minDepthInt = asuint(minDepth);
        uint maxDepthInt = asuint(maxDepth);

        InterlockedMin(reducedDepth[0].minDepth, minDepthInt);
        InterlockedMax(reducedDepth[0].maxDepth, maxDepthInt);
    }
}
