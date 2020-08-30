#include "../common/vertexDefinitions.hlsl"

Texture2D sourceTexture: register(t4,space3);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

    float4 color = float4(sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz*0.3f,1.0f);
    return color;
}


