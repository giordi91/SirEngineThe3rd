#ifndef DEFERRED_HLSL 
#define DEFERRED_HLSL

static const float2 g_SpecPowerRange = {10.0, 250.0};

struct PS_GBUFFER_OUT {
  float4 ColorSpecInt : SV_TARGET0;
  float4 Normal : SV_TARGET1;
  float4 SpecPow : SV_TARGET2;
};

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
  float thickness;
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

inline float ConvertZToLinearDepth(float depth) {
  float linearDepth = g_cameraBuffer.perspectiveValues.z /
                      (depth + g_cameraBuffer.perspectiveValues.w);
  return linearDepth;
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
  float4 spec = specPowTexture.Sample(gsamPointClamp, UV.xy);
  Out.specPow = spec.x;
  Out.metallic = spec.y;
  Out.roughness = spec.z;
  Out.thickness = spec.w;

  return Out;
}


#endif 