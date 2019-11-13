#ifndef SHADOWS_HLSL
#define SHADOWS_HLSL

float samplePCF9taps(float3 worldPos, float4x4 lightVP) {

	//lets check shadows
	float4 shadowMapPos = mul(float4(worldPos,1.0), lightVP);
	shadowMapPos  /= shadowMapPos.w;
	//convert to uv values
	float2 shadowUV= 0.5f * shadowMapPos.xy + 0.5f;
	//float2 shadowUV= uv/4;
	shadowUV.y = 1.0f - shadowUV.y;

	//sample the shadow
    float attenuation=0.0f;
	
	float dx = 1.0f / 4096.0f;
	const	float2	offsets[9]	=		
	{				
		float2(-dx,	-dx),	
		float2(0.0f,	-dx),	
		float2(dx,	dx),				
		float2(-dx,	0.0f),	
		float2(0.0f,	0.0f),	
		float2(dx, 0.0f),				
		float2(-dx,	+dx),	
		float2(0.0f,	+dx),	
		float2(dx, +dx)		
	};
	[unroll]
	for(int i =0; i < 9;++i)
	{
		attenuation+=directionalShadow.SampleCmpLevelZero(shadowPCFClamp,shadowUV + offsets[i],shadowMapPos.z).x;
	}
	attenuation/=9.0f;
	return attenuation;
}

#endif