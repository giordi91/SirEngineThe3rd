#include "../common/deferredPacking.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<PhongMaterial> g_material : register(b1);
Texture2D albedoTex : register(t0);
Texture2D tangentTex: register(t1);
Texture2D metallicTex: register(t2);
Texture2D roughnessTex : register(t3);
Texture2D thicknessTex : register(t4);

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
      normalize(tangentTex.Sample(gsamLinearClamp, uv) * 2.0f - 1.0f);
  // compute NTB
  float3 N = normalize(input.Normal.xyz);
  float3 T = normalize(input.tangent.xyz);
  float3 B = normalize(cross(N, T));
  float3x3 NTB;
  float3 normal = normalize(float3(tanN.x * T) + (tanN.y*B) + (tanN.z*N));
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
