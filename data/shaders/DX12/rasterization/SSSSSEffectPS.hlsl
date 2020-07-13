#include "../common/structures.hlsl"

struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

Texture2D sourceTexture : register(t0,space3);
Texture2D depthTex : register(t1,space3);
ConstantBuffer<CameraBuffer> g_camera: register(b0, space0);
ConstantBuffer<SSSSSConfig> g_config: register(b0, space3);

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

  float2 pixelSize = float2(1.0f, 1.0f) / float2(g_camera.screenWidth, g_camera.screenHeight);

  float depth = ( depthTex.Sample(gsamPointClamp, pin.uv).r);
  depth = ConvertZToLinearDepth(depth); 
  float dd = g_config.direction.x > 0.1f ? min(abs(ddx(depth)), g_config.maxdd) : min(abs(ddy(depth)), g_config.maxdd);
  float2 s_x = g_config.sssLevel / (depth + g_config.correction * dd);
  float2 finalWidth = s_x * g_config.width * pixelSize * g_config.direction;

  //finalWidth = float2(0.01f,0.0f);
  float2 offset = pin.uv - finalWidth;
  float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
  for (int i = 0; i < 7; ++i) {
    float3 tap = sourceTexture.Sample(gsamLinearClamp, offset).rgb;
    color.rgb += w[i] * tap;
    //color.rgb += (1.0f/7.0f) * tap;
    offset += finalWidth / 3.0f;
  }

  //float3 red = float3(1,0,0);
  //float3 green = float3(0,1,0);
  //color.rgb = s_x.y > 20.0f ? red : green;
  //color.rgb = float3(finalWidth,0)*20.0f;
  //color.rgb = sourceTexture.Sample(gsamLinearClamp, pin.uv).rgb;
  return color;
}
