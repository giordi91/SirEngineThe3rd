#include "../RaytracingHlslCompat.h"
ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1);

// deferred buffer bindings
Texture2D depthTexture : register(t0);
Texture2D colorSpecIntTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D specPowTexture : register(t3);
Texture2D reflectionTexture : register(t4);
Texture2D reflectionTextureStencil : register(t5);


SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
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
  Out.specPow = specPowTexture.Sample(gsamPointClamp, UV.xy).x;

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

float4 PS(VertexOut input) : SV_TARGET {
  SURFACE_DATA gbd = UnpackGBuffer(input.uv);

  float3 ldir = normalize(-g_dirLight.lightDir.xyz);
  float4 finalColor = float4(gbd.color, 0.0f);

  float isReflective =
      reflectionTextureStencil.Sample(gsamPointClamp, input.uv);
  if (isReflective) {
    finalColor = reflectionTexture.Sample(gsamPointClamp, input.uv);
    finalColor.w = 1.0f;
  }

  if (gbd.depth <= MAX_DEPTH) {
    finalColor = finalColor * saturate(dot(ldir, gbd.normal));

    float3 worldPos = CalcWorldPos(input.clipPos, gbd.linearDepth);
    // float shadowAttenuation = SpotShadowPCF(worldPos, g_dirLight.lightVP);

    // compute specular
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float specularValue = saturate(dot(halfWay, gbd.normal));
    float specP = gbd.specPow * (250.0f - 10.0f) + 10.0f;
    float3 finalSpecular =
        (g_dirLight.lightColor * pow(specularValue, specP) * gbd.specIntensity)
            .xyz;

    // final color
    finalColor.x += finalSpecular.x;
    finalColor.y += finalSpecular.y;
    finalColor.z += finalSpecular.z;

    // TODO(fix hardcoded ambient)
    float3 ambient = 0.25f;
    // float ao = AOTexture.Sample(gsamLinearClamp, input.uv);
    float shadowAttenuation = 1.0f;
    finalColor.xyz =
        (finalColor.xyz * shadowAttenuation) + (ambient * gbd.color);
    finalColor.w = 1.0f;
  }

  return finalColor;
}
