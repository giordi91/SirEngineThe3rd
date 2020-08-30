#include "../common/vertexDefinitions.hlsl"

Texture2D albedoTex : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = albedoTex.Sample(gsamAnisotropicWrap, pin.uv);
	return diffuseAlbedo;
}


