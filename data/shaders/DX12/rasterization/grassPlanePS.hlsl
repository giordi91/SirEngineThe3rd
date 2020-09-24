#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

ConstantBuffer<GrassConfig> grassConfig: register(b3,space3);
Texture2D sourceTexture: register(t4,space3);

SamplerState gsamPointWrap        : register(s0,space1);
SamplerState gsamPointClamp       : register(s1,space1);
SamplerState gsamLinearWrap       : register(s2,space1);
SamplerState gsamLinearClamp      : register(s3,space1);
SamplerState gsamAnisotropicWrap  : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

ConstantBuffer<FrameData> g_frameData : register(b0,space0);
ConstantBuffer<DirectionalLightData> g_dirLightData : register(b0, space2);
TextureCube skyboxIrradianceTexture : register(t1, space2);
TextureCube skyboxRadianceTexture : register(t2, space2);
Texture2D brdfTexture : register(t3, space2);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

    float4 color = float4(sourceTexture.Sample(gsamLinearClamp, pin.uv).xyz*0.3f,1.0f);
    return color;
}


