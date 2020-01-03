#include "../common/deferredPacking.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<PhongMaterial> g_material : register(b1,space1);
Texture2D albedoTex : register(t0,space1);
Texture2D tangentTex: register(t1,space1);
Texture2D metallicTex: register(t2,space1);
Texture2D roughnessTex : register(t3,space1);
Texture2D thicknessTex : register(t4,space1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);


PS_GBUFFER_OUT PS(FullMeshVertexOut input) {

  // Lookup mesh texture and modulate it with diffuse
	float2 uv = float2(input.uv.x , 1.0f- input.uv.y);
  float3 DiffuseColor =
      albedoTex.Sample(gsamLinearClamp, uv).xyz * g_material.kd.xyz;


  float4 tanN =
      tangentTex.Sample(gsamLinearClamp, uv) * 2.0f - 1.0f;
  // compute NTB
  float3 N = normalize(input.Normal.xyz);
  float3 T = normalize(input.tangent.xyz);
  float3 B = normalize(cross(N, T));
    //float3x3 NTB = transpose(float3x3(N, T, B));
    float3x3 NTB = float3x3(T, B,N);
  //float3 normal = normalize(float3(tanN.x * T) + (tanN.y*B) + (tanN.z*N));
  float3 normal = normalize(mul(tanN.xyz, NTB));
  //float3 normal = normalize(mul(NTB,tanN.xyz ));
  //float3 normal = 0.0f;
  //normal = input.Normal.xyz;

  //sampling PBR textures
  float metallic = 
      metallicTex.Sample(gsamLinearClamp, uv).x;
  float roughness= 
      roughnessTex.Sample(gsamLinearClamp, uv).x;
  float thickness= 
      thicknessTex.Sample(gsamLinearClamp, uv).r;


  return PackGBuffer(DiffuseColor, normal, g_material.ks.x,metallic,roughness,
                     g_material.shiness,thickness);
}
