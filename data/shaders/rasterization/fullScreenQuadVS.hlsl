
struct CameraBuffer {
  float4x4 MVP;
  float4x4 ViewMatrix;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;
};

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

static const float4 arrBasePos[6] = {
    float4(-1.0f, 1.0f, 0.0f, 1.0f), float4(1.0f, 1.0f, 1.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 0.0f), float4(1.0f, -1.0f, 1.0f, 0.0f),
	float4(-1.0f, -1.0f, 0.0f, 0.0f),float4(1.0f, 1.0f, 1.0f, 1.0f)};

VertexOut VS( uint vid : SV_VertexID) {
  VertexOut vout;
  float4 p = arrBasePos[vid];

  vout.pos.xy = p.xy;
  vout.pos.z = 0.0f;
  vout.pos.w = 1.0f;
  vout.clipPos = p.xy; 
  vout.uv = p.zw;
  return vout;
}
