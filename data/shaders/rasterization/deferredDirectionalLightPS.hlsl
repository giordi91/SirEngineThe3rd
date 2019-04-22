#include "../common/deferred.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1);

// deferred buffer bindings
Texture2D depthTexture : register(t0);
Texture2D colorSpecIntTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D specPowTexture : register(t3);
TextureCube skyboxIrradianceTexture: register(t4);
TextureCube skyboxRadianceTexture: register(t5);
Texture2D brdfTexture: register(t6);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static const float PI = 3.14159265359f;

// data returned from the gbuffer
struct SURFACE_DATA {
  float linearDepth;
  float3 color;
  float3 normal;
  float specIntensity;
  float specPow;
  float depth;
};
struct SURFACE_DATA_PBR {
  float linearDepth;
  float3 color;
  float3 normal;
  float specIntensity;
  float specPow;
  float depth;
  float metallic;
  float roughness;
};

inline float ConvertZToLinearDepth(float depth) {
  float linearDepth = g_cameraBuffer.perspectiveValues.z /
                      (depth + g_cameraBuffer.perspectiveValues.w);
  return linearDepth;
}

inline SURFACE_DATA UnpackGBuffer(float2 UV) {
  SURFACE_DATA Out;

  float depth = depthTexture.Sample(gsamPointClamp, UV.xy).x;
  Out.linearDepth = ConvertZToLinearDepth(depth);
  Out.depth = depth;

  float4 baseColorSpecInt = colorSpecIntTexture.Sample(gsamPointClamp, UV.xy);
  Out.color = baseColorSpecInt.xyz;
  Out.specIntensity = baseColorSpecInt.w;
  Out.normal = normalTexture.Sample(gsamPointClamp, UV.xy).xyz;
  Out.normal = normalize(Out.normal * 2.0 - 1.0);
  // Out.normal = DecodeOctNormal(normalTexture.Sample(gsamPointClamp,
  // UV.xy).xy);
  Out.specPow = specPowTexture.Sample(gsamPointClamp, UV.xy).x;

  return Out;
}

inline SURFACE_DATA_PBR UnpackGBufferPBR(float2 UV) {
  SURFACE_DATA_PBR Out;

  float depth = depthTexture.Sample(gsamPointClamp, UV.xy).x;
  Out.linearDepth = ConvertZToLinearDepth(depth);
  Out.depth = depth;

  float4 baseColorSpecInt = colorSpecIntTexture.Sample(gsamPointClamp, UV.xy);
  Out.color = baseColorSpecInt.xyz;
  Out.specIntensity = baseColorSpecInt.w;
  Out.normal = normalTexture.Sample(gsamPointClamp, UV.xy).xyz;
  Out.normal = normalize(Out.normal * 2.0 - 1.0);
  // Out.normal = DecodeOctNormal(normalTexture.Sample(gsamPointClamp,
  // UV.xy).xy);
  float3 spec = specPowTexture.Sample(gsamPointClamp, UV.xy).xyz;
  Out.specPow = spec.x;
  Out.metallic = spec.y;
  Out.roughness = spec.z;

  return Out;
}
float3 CalcWorldPos(float2 csPos, float depth) {
  float4 position;

  position.xy = csPos.xy * g_cameraBuffer.perspectiveValues.xy * depth;
  position.z = depth;
  position.w = 1.0;

  return mul(position, g_cameraBuffer.ViewMatrix).xyz;
}

#define MAX_DEPTH 0.999999f

float4 phongLighting(FullScreenVertexOut input) {
  SURFACE_DATA gbd = UnpackGBuffer(input.uv);

  float3 ldir = normalize(-g_dirLight.lightDir.xyz);
  float4 finalColor = float4(gbd.color, 0.0f);

  if (gbd.depth <= MAX_DEPTH) {
    finalColor = finalColor * saturate(dot(ldir, gbd.normal));

    float3 worldPos = CalcWorldPos(input.clipPos, gbd.linearDepth);
    // float shadowAttenuation = SpotShadowPCF(worldPos, g_dirLight.lightVP);

    // compute specular
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float specularValue = saturate(dot(halfWay, gbd.normal));
    float specP = gbd.specPow * (250.0f - 10.0f) + 10.0f;
    // float specP = 0.5f * (250.0f - 10.0f) + 10.0f;
    float3 finalSpecular =
        (g_dirLight.lightColor * pow(specularValue, specP) * gbd.specIntensity)
            .xyz;

    // final color
    finalColor.x += finalSpecular.x;
    finalColor.y += finalSpecular.y;
    finalColor.z += finalSpecular.z;

    // TODO(fix hardcoded ambient)
    float3 ambient = 0.03f;
    // float ao = AOTexture.Sample(gsamLinearClamp, input.uv);
    float shadowAttenuation = 1.0f;
    finalColor.xyz =
        (finalColor.xyz * shadowAttenuation) + (ambient * gbd.color);
    finalColor.w = 1.0f;
  }
  return finalColor;
}
//==============================================
// PBR
//==============================================

float3 fresnelSchlick(float cosTheta, float3 F0,float roughness) {
  //return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
  float3 oneMinusRoughness =1.0 - roughness;  
  return F0 + (max(oneMinusRoughness, F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;
  return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return nom / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);
  return ggx1 * ggx2;
}

float4 PBRLighting(FullScreenVertexOut input) {
  SURFACE_DATA_PBR gbd = UnpackGBufferPBR(input.uv);
  float4 finalColor = 0.0f;
  float3 ldir = normalize(-g_dirLight.lightDir.xyz);

  if (gbd.depth <= MAX_DEPTH) {
    float3 worldPos = CalcWorldPos(input.clipPos, gbd.linearDepth);
    // camera vector
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float3 reflected = reflect(-toEyeDir, gbd.normal);
    //reflected = gbd.normal;


    // fresnel slick, ratio between specular and diffuse, it is tintend on
    // metal, so we lerp toward albedo based on metallic, so only specular will
    // be tinted by the albedo.
    float3 F0 = 0.04f;

	float3 albedo =gbd.color; 
	float metallic = gbd.metallic;
	float roughness = gbd.roughness;


    float MAX_REFLECTION_LOD = 6.0f;
	float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflected, roughness * MAX_REFLECTION_LOD).rgb;
	//float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflected, 0).rgb;


	//float3 albedo =float3(0.8f,0.0f,0.0f); 
	//float metallic = 0.0f;
	//float roughness = 1.0f;

    F0 = lerp(F0, albedo, metallic);
    float3 F = fresnelSchlick(max(dot(halfWay, toEyeDir), 0.0f), F0, roughness);
    float NDF = DistributionGGX(gbd.normal, halfWay, roughness);
    float G = GeometrySmith(gbd.normal, toEyeDir, ldir, roughness);

    float2 envBRDF = brdfTexture.Sample(gsamLinearClamp, float2(max(dot(gbd.normal, toEyeDir), 0.0), roughness)).rg;
    float3 specularDiff = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    // compute cook torrance
    float3 nominator = NDF * G * F;
    float denominator =
        4.0f * max(dot(gbd.normal, toEyeDir), 0.0f) * max(dot(gbd.normal, ldir), 0.0f) + 0.001f;
    float3 specular = nominator / denominator;

    // compute specular
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;

    // reflectance value
    float3 Lo = 0.0f;
    // single light for now
    float3 radiance = g_dirLight.lightColor.xyz;
    float NdotL = max(dot(gbd.normal, ldir), 0.0f);
    Lo += (kD * albedo/ PI + specular) * radiance * NdotL;


	//using irradiance map
	float3 irradiance = skyboxIrradianceTexture.Sample(gsamLinearClamp,gbd.normal);
	float3 diffuse      = irradiance* albedo;
	float3 ambient = kD * diffuse + specularDiff;

    float3 color = ambient + Lo;
    //float3 color = irradiance*kD;



        //return float4(prefilteredColor, 1.0f);
    //return float4(envBRDF,0.0f, 1.0f);
    return float4(color, 1.0f);
  }
  return finalColor;
}

float4 PS(FullScreenVertexOut input) : SV_TARGET {
  //return phongLighting(input);
  return PBRLighting(input);
}
