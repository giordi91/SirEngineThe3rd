#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

// deferred buffer bindings
TextureCube skyboxTexture: register(t1,space3);

SamplerState gsamPointWrap : register(s0, space1);
SamplerState gsamPointClamp : register(s1, space1);
SamplerState gsamLinearWrap : register(s2,space1);
SamplerState gsamLinearClamp : register(s3,space1);
SamplerState gsamAnisotropicWrap : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

float4 PS(PositionOnlyVertexOut input) : SV_TARGET {
  return skyboxTexture.Sample(gsamLinearClamp,input.worldPos.xyz);
}
