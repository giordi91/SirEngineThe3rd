#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

Texture2D sourceTexture: register(t0,space3);
ConstantBuffer<GammaToneMappingConfig> g_config: register(b1,space3);

SamplerState gsamPointWrap        : register(s0,space1);
SamplerState gsamPointClamp       : register(s1,space1);
SamplerState gsamLinearWrap       : register(s2,space1);
SamplerState gsamLinearClamp      : register(s3,space1);
SamplerState gsamAnisotropicWrap  : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

float4 PS(FullScreenVertexOut pin) : SV_Target
{
    float3 color = sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz;
	//exposure tone mapping
	float3 mapped = float3(1.0f,1.0f,1.0f) -exp(-color* g_config.exposure);
	//gamma
	mapped = pow(mapped, g_config.gammaInverse);
	return float4(mapped,1.0f);
}


