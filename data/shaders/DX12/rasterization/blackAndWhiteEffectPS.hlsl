struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

Texture2D sourceTexture: register(t0,space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(VertexOut pin) : SV_Target
{

    float4 color = sourceTexture.Sample(gsamLinearClamp, pin.uv);
	return (color.x* 0.3f + color.y* 0.59f + color.z* 0.11f);
}


