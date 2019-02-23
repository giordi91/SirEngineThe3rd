#include "../common/vertexDefinitions.hlsl"



Texture2D sourceTexture: register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

	float DepthMinRange = 0.9997f;
	float DepthMaxRange = 0.9998f;
    float depth = sourceTexture.Sample(gsamPointClamp, pin.uv).x;
	//remapping depth
	depth = saturate(abs(depth - DepthMinRange) / (DepthMaxRange - DepthMinRange));
	return float4(depth, 0, 0, 1);
}


