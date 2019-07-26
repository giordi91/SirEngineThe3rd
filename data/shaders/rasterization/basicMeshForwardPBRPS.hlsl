#include "../common/deferred.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<PhongMaterial> g_material : register(b1);
Texture2D albedoTex : register(t0);
Texture2D tangentTex: register(t1);
Texture2D metallicTex: register(t2);
Texture2D roughnessTex : register(t3);

static const float2 g_SpecPowerRange = {10.0, 250.0};

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut input): SV_Target
{ 

	return float4(1.0f,0.0f,0.0f,1.0f);
}
