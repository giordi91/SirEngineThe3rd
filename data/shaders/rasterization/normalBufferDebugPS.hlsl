#include "../common/vertexDefinitions.hlsl"
#include "../common/deferred.hlsl"
Texture2D sourceTexture: register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

    float4 color = float4(sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz,1.0f);
	color = (color*2.0f) - 1.0f;
	color.w = 1.0f;
	//color.xyz = DecodeOctNormal(color.xy);
	//color.w =1.0f;
	return color;
}


