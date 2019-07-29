#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

// deferred buffer bindings
TextureCube skyboxTexture: register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(PositionOnly input) : SV_TARGET {
  return skyboxTexture.Sample(gsamLinearClamp,input.worldPos.xyz);
  //return float4(1,0,0,1);
}
