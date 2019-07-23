#include "../common/structures.hlsl"

struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

Texture2D sourceTexture : register(t0);
Texture2D depthTex : register(t1);
ConstantBuffer<CameraBuffer> g_camera: register(b0);
ConstantBuffer<SSSSSConfig> g_sssss : register(b1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

inline float ConvertZToLinearDepth(float depth) {
  float linearDepth = g_camera.perspectiveValues.z / (depth + g_camera.perspectiveValues.w);
  return linearDepth;
}
float4 PS(VertexOut pin) : SV_Target {
  float w[7] = {0.006, 0.061, 0.242, 0.382, 0.242, 0.061, 0.006};

  float sssLevel = 31.5f;
  float correction = 800.0f;
  float maxdd = 0.001f;
  float width = 0.5f;
  float2 pixelSize = float2(1.0f, 1.0f) / float2(1280.0f, 720.0f);

  float depth = ( depthTex.Sample(gsamPointClamp, pin.uv).r);
  depth = ConvertZToLinearDepth(depth); 
  float2 s_x = sssLevel / (depth + correction * min(abs(ddx(depth)), maxdd));
  float2 finalWidth = s_x * width * pixelSize * float2(1.0, 0.0);

  float2 offset = pin.uv - finalWidth;
  float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
  for (int i = 0; i < 7; ++i) {
    float3 tap = sourceTexture.Sample(gsamLinearClamp, offset).rgb;
    color.rgb += w[i] * tap;
    offset += finalWidth / 3.0f;
  }

  //color.rgb = float3(depth,0,0);
  //color.rgb = sourceTexture.Sample(gsamLinearClamp, pin.uv).rgb;
  return color;
}
