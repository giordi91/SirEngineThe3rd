#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"
#include "../common/pbr.hlsl"
#include "../common/normals.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
ConstantBuffer<DirectionalLightData> g_dirLightData : register(b0, space2);

Texture2D albedoTex : register(t4, space3);
Texture2D tangentTex : register(t5, space3);
Texture2D metallicTex : register(t6, space3);
Texture2D roughnessTex : register(t7, space3);

TextureCube skyboxIrradianceTexture : register(t1, space2);
TextureCube skyboxRadianceTexture : register(t2, space2);
Texture2D brdfTexture : register(t3, space2);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullMeshVertexOut pin) : SV_Target {
  float2 uv = float2(pin.uv.x, 1.0f - pin.uv.y);
  float3 albedo = albedoTex.Sample(gsamLinearClamp, uv).xyz;

  float3 texNormal =
      normalize(tangentTex.Sample(gsamLinearClamp, uv) * 2.0f - 1.0f).xyz;
  // compute NTB
  float3 N = normalize(pin.Normal.xyz);
  float3 T = normalize(pin.tangent.xyz);
  float3 normal = computeNormalFromNormalMap(N, T, texNormal);

  // sampling PBR textures
  float metallic = metallicTex.Sample(gsamLinearClamp, uv).x;
  float roughness = roughnessTex.Sample(gsamLinearClamp, uv).x;

  // view vectors
  float3 worldPos = pin.worldPos.xyz;
  // camera vector
  float3 ldir = normalize(-g_dirLightData.lightDir.xyz);
  float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
  float3 halfWay = normalize(toEyeDir + ldir);
  float3 reflected = reflect(-toEyeDir, pin.Normal);

  float4 color =
      albedoTex.Sample(gsamLinearClamp, float2(pin.uv.x, 1.0f - pin.uv.y));
  // return color;
  float3 l = -g_dirLightData.lightDir.xyz;
  float d = dot(l, pin.Normal);
  return float4(normal, 1.0f);
}
