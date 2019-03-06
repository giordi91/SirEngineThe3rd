#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"


Texture2D sourceTexture: register(t0);
ConstantBuffer<GammaToneMappingConfig> g_config: register(b1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullScreenVertexOut pin) : SV_Target
{
    float3 color = sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz;
	//exposure tone mapping
	float3 mapped = float3(1.0f,1.0f,1.0f) -exp(-color* g_config.exposure);
	//gamma
	mapped = pow(mapped, g_config.gammaInverse);
	return float4(mapped,1.0f);
}


