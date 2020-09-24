#include "../common/vertexDefinitions.hlsl"

Texture2D sourceTexture: register(t4,space3);

SamplerState gsamPointWrap        : register(s0,space1);
SamplerState gsamPointClamp       : register(s1,space1);
SamplerState gsamLinearWrap       : register(s2,space1);
SamplerState gsamLinearClamp      : register(s3,space1);
SamplerState gsamAnisotropicWrap  : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

    float4 color = float4(sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz*0.3f,1.0f);
    return color;
}


