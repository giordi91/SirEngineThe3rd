#include "../common/deferred.hlsl"
#include "../common/normals.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1);
ConstantBuffer<PhongMaterial> g_material : register(b2);
Texture2D albedoTex : register(t0);
Texture2D tangentTex : register(t1);
Texture2D metallicTex : register(t2);
Texture2D roughnessTex : register(t3);

static const float2 g_SpecPowerRange = {10.0, 250.0};

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut input) : SV_Target {
  float2 uv = float2(input.uv.x, 1.0f - input.uv.y);
  float3 diffuseColor =
      albedoTex.Sample(gsamLinearClamp, uv).xyz * g_material.kd.xyz;

  float3 texNormal =
      normalize(tangentTex.Sample(gsamLinearClamp, uv) * 2.0f - 1.0f).xyz;
  // compute NTB
  float3 N = normalize(input.Normal.xyz);
  float3 T = normalize(input.tangent.xyz);
  float3 normal = computeNormalFromNormalMap(N, T, texNormal);

  // sampling PBR textures
  float metallic =
      metallicTex.Sample(gsamLinearClamp, uv).x * g_material.metallicMult;
  float roughness =
      roughnessTex.Sample(gsamLinearClamp, uv).x * g_material.roughnessMult;

  // view vectors
  float3 worldPos = input.worldPos.xyz;

  // camera vector
  float3 ldir = normalize(-g_dirLight.lightDir.xyz);
  float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
  float3 halfWay = normalize(toEyeDir + ldir);
  float3 reflected = reflect(-toEyeDir, input.Normal);

  return float4(normal.x, metallic, reflected.x, g_dirLight.lightDir.x);
}
