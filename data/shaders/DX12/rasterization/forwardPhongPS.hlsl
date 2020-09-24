#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"
#include "../common/textureSampling.hlsl"
#include "../common/pbr.hlsl"
#include "../common/normals.hlsl"

ConstantBuffer<FrameData> g_frameData : register(b0,space0);
ConstantBuffer<DirectionalLightData> g_dirLightData : register(b0, space2);

Texture2D albedoTex : register(t4, space3);
Texture2D tangentTex : register(t5, space3);
Texture2D metallicTex : register(t6, space3);
Texture2D roughnessTex : register(t7, space3);
ConstantBuffer<ForwardPBRMaterial> materialConfig : register(b8,space3);

TextureCube skyboxIrradianceTexture : register(t1, space2);
TextureCube skyboxRadianceTexture : register(t2, space2);
Texture2D brdfTexture : register(t3, space2);

SamplerState gsamPointWrap : register(s0,space1);
SamplerState gsamPointClamp : register(s1,space1);
SamplerState gsamLinearWrap : register(s2,space1);
SamplerState gsamLinearClamp : register(s3,space1);
SamplerState gsamAnisotropicWrap : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);

float4 PS(FullMeshVertexOut pin) : SV_Target {
  float2 uv = float2(pin.uv.x, 1.0f - pin.uv.y);
  float3 albedo = albedoTex.Sample(gsamLinearWrap, remapUV(uv,materialConfig.albedoTexConfig)).xyz;

  float3 texNormal =
      normalize(tangentTex.Sample(gsamLinearWrap, remapUV(uv,materialConfig.normalTexConfig)) * 2.0f - 1.0f).xyz;
  // compute NTB
  float3 N = normalize(pin.Normal.xyz);
  float3 T = normalize(pin.tangent.xyz);
  float3 normal = computeNormalFromNormalMap(N, T, texNormal);

  // sampling PBR textures
  float metallic = metallicTex.Sample(gsamLinearWrap, remapUV(uv,materialConfig.metallicTexConfig)).x *materialConfig.metalRoughMult.x;
  float roughness = roughnessTex.Sample(gsamLinearWrap, remapUV(uv,materialConfig.roughnessTexConfig)).x*materialConfig.metalRoughMult.y;

  // view vectors
  float3 worldPos = pin.worldPos.xyz;
  // camera vector
  float3 ldir = normalize(-g_dirLightData.lightDir.xyz);
  float3 toEyeDir = normalize(g_frameData.m_activeCamera.position.xyz - worldPos);
  float3 halfWay = normalize(toEyeDir + ldir);
  float3 reflected = reflect(-toEyeDir, pin.Normal);
 // fresnel slick, ratio between specular and diffuse, it is tintend on
  // metal, so we lerp toward albedo based on metallic, so only specular will
  // be tinted by the albedo.
  float3 F0 = 0.04f;
  F0 = lerp(F0, albedo, metallic);
  float3 F = fresnelSchlick(max(dot(normal, toEyeDir), 0.0f), F0, roughness);
  float NDF = DistributionGGX(normal, halfWay, roughness);
  float G = GeometrySmith(normal, toEyeDir, ldir, roughness);

  // compute cook torrance
  float3 nominator = NDF * G * F;
  float denominator =
  	4.0f * max(dot(normal, toEyeDir), 0.0f) * max(dot(normal, ldir), 0.0f) + 0.001f;
  float3 specular = nominator / denominator;
  
  // compute specular
  float3 kS = F;
  float3 kD = 1.0f - kS;
  kD *= 1.0f - metallic;
  
  // reflectance value
  float3 Lo = 0.0f;
  // single light for now
  float3 radiance = g_dirLightData.lightColor.xyz;
  float NdotL = max(dot(normal, ldir), 0.0f);
  	Lo += (kD * (albedo / PI) + specular) * radiance * NdotL;
  
  //using irradiance map
  float3 irradiance = skyboxIrradianceTexture.Sample(gsamLinearClamp,normal).xyz;
  float3 diffuse      = irradiance* albedo;
  
  float MAX_REFLECTION_LOD = 8.0f;
  float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflected, roughness* MAX_REFLECTION_LOD).rgb;
  float2 envBRDF = brdfTexture.Sample(gsamLinearClamp, float2(max(dot(normalize(normal), normalize(toEyeDir)), 0.0), roughness)).rg;
  float3 specularDiff = prefilteredColor * (F* envBRDF.x + envBRDF.y);

  float3 ambient = (kD * diffuse) + specularDiff;
  float3 color = ambient + Lo;

  return float4(color, 1.0f);

  /*
  float4 color =
      albedoTex.Sample(gsamLinearClamp, float2(pin.uv.x, 1.0f - pin.uv.y));
  // return color;
  float3 l = -g_dirLightData.lightDir.xyz;
  float d = dot(l, pin.Normal);
  return float4(normal, 1.0f);
  */
}
