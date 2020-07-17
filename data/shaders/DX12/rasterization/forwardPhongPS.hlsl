#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

Texture2D sourceTexture: register(t3,space3);
ConstantBuffer<DirectionalLightData> g_dirLightData: register(b0,space2);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut pin) : SV_Target
{
    float4 color = sourceTexture.Sample(gsamLinearClamp, float2(pin.uv.x, 1.0f - pin.uv.y));
    //return color;
    float3 l = -g_dirLightData.lightDir.xyz;
    float d= dot(l,pin.Normal);
	return float4(color.xyz*d,1.0f);
}


