Texture2D<float4> Input : register( t0 );
RWTexture2D<float4> Output : register( u0 );


//ConstantBuffer<SmoothConfig> g_config: register(b0);

[numthreads( 8, 8, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{
	Output[id.xy] = float4(float(id.x)/1024.0f,float(id.y)/1024.0f,0.0f,1.0f);
}