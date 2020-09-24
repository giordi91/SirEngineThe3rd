struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

Texture2D sourceTexture: register(t0,space3);

SamplerState gsamPointWrap        : register(s0,space1);
SamplerState gsamPointClamp       : register(s1,space1);
SamplerState gsamLinearWrap       : register(s2,space1);
SamplerState gsamLinearClamp      : register(s3,space1);
SamplerState gsamAnisotropicWrap  : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = sourceTexture.Sample(gsamLinearWrap, pin.uv);
	return color = color;
}


