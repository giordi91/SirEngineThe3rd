#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

static const float4 arrBasePos[6] = {
    float4(-1.0f, 1.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 0.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f), float4(1.0f, -1.0f, 1.0f, 1.0f),
	float4(-1.0f, -1.0f, 0.0f, 1.0f),float4(1.0f, 1.0f, 1.0f, 0.0f)};

FullScreenVertexOut VS( uint vid : SV_VertexID) {
  FullScreenVertexOut vout;
  float4 p = arrBasePos[vid];

  vout.pos.xy = p.xy;
  vout.pos.z = 0.00001f;
  vout.pos.w = 1.0f;
  vout.clipPos = p.xy; 
  vout.uv = p.zw;
  return vout;
}
