#include "../common/structures.hlsl"

Texture2D<float> Input : register( t0 );
ConstantBuffer<TextureConfig> g_textureConfig: register(b0);
RWStructuredBuffer<ReducedDepth> reducedDepth : register(u0);

[numthreads( 1, 1, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{
    reducedDepth[0].minDepth = 1.0f;
    reducedDepth[0].maxDepth= 0.0f;
}
