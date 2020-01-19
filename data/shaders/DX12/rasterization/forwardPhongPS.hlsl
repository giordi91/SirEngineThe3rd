#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

Texture2D sourceTexture: register(t3,space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut pin) : SV_Target
{
    float4 color = sourceTexture.Sample(gsamLinearClamp, pin.uv);
	return color;
}


