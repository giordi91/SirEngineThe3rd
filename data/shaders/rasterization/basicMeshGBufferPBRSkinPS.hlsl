#include "../common/deferred.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<PhongMaterial> g_material : register(b1);
Texture2D albedoTex : register(t0);
Texture2D tangentTex: register(t1);
Texture2D metallicTex: register(t2);
Texture2D roughnessTex : register(t3);
Texture2D thicknessTex : register(t4);

static const float2 g_SpecPowerRange = {10.0, 250.0};

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct PS_GBUFFER_OUT {
  float4 ColorSpecInt : SV_TARGET0;
  float4 Normal : SV_TARGET1;
  float4 SpecPow : SV_TARGET2;
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float SpecIntensity,
	float metallic, float roughness, float SpecPower, float thickness) {
  PS_GBUFFER_OUT Out;

  // Normalize the specular power
  float SpecPowerNorm =
      saturate((SpecPower - g_SpecPowerRange.x) / g_SpecPowerRange.y);

  // Pack all the data into the GBuffer structure
  Out.ColorSpecInt = float4(BaseColor.rgb, SpecIntensity);
  Out.Normal = float4(Normal * 0.5f + 0.5f, 1.0f);
  // Out.Normal.xy = EncodeOctNormal(Normal);
  // Out.Normal.zw = 0.0f;
  Out.SpecPow = float4(SpecPowerNorm, metallic, roughness, thickness);

  return Out;
}
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
      metallicTex.Sample(gsamLinearClamp, uv).xyz;
  float roughness= 
      roughnessTex.Sample(gsamLinearClamp, uv).xyz;
  float thickness= 
      thicknessTex.Sample(gsamLinearClamp, uv).r;


  return PackGBuffer(DiffuseColor, normal, g_material.ks.x,metallic,roughness,
                     g_material.shiness,thickness);
}
