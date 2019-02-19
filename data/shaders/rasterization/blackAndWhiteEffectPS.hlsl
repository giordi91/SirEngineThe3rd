struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

Texture2D sourceTexture: register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(VertexOut pin) : SV_Target
{

    float4 color = sourceTexture.Sample(gsamLinearClamp, pin.uv);
	return color*0.5f;
	return float4(1,0,0,1);
}


