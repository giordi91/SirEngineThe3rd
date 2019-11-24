#ifndef DEFERRED_HLSL 
#define DEFERRED_HLSL

static const float2 g_SpecPowerRange = {10.0, 250.0};

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
#endif 