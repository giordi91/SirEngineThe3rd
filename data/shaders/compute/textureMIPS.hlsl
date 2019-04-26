Texture2D<float4> Input : register( t0 );
RWTexture2D<float4> Output : register( u0 );


//ConstantBuffer<SmoothConfig> g_config: register(b0);

[numthreads( 8, 8, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{
	//Output[id.xy] = float4(float(id.x)/1024.0f,float(id.y)/1024.0f,0.0f,1.0f);
	Output[id.xy] = Input[id.xy];

	int x2 = id.x * 2;
	int y2 = id.y * 2;
	float4 v = Input[float2(x2, y2)] + Input[float2(x2 + 1, y2)]+
	+ Input[float2(x2, y2 + 1)]+  Input[float2(x2 + 1, y2 + 1)];
	Output[id.xy] = v*0.25;
}