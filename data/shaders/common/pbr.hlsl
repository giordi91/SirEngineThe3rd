#ifndef PBR_HLSL
#define PBR_HLSL

static const float PI = 3.14159265359f;

float3 fresnelSchlick(float cosTheta, float3 F0, float roughness) {
  // return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
  float3 oneMinusRoughness = 1.0 - roughness;
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

#endif