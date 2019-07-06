#include "../common/structures.hlsl"

Texture2D<float> Input : register( t0 );
ConstantBuffer<TextureConfig> g_textureConfig: register(b0);
RWStructuredBuffer<ReducedDepth> reducedDepth : register(u0);

[numthreads( 32, 4, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{
    float depth = Input[id.xy];
    float minDepth = WaveActiveMin(depth);
    //float maxDepth = WaveAllMax(depth);

    uint minDepthInt = asuint(minDepth);
    //uint minDepthInt = asuint(minDepth);
    reducedDepth[0].minDepth = minDepth;
	//Output[id.xy] = Input[id.xy];
}
