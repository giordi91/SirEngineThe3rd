struct CameraBuffer {
  float4x4 MVP;
  float4x4 ViewMatrix;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;
};

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
//cbuffer cbPerObject : register(b0)
//{
//	float4x4 gWorldViewProj; 
//};


struct VertexIn
{
	float4 PosL  : POSITION;
    float4 Normal : NORMAL;
    float4 uvs: TEXCOORD;
    float4 tangents: TANGENTS;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Normal: NORMAL;
	float2 uv :TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(vin.PosL, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= vin.Normal;
	vout.uv = vin.uvs.xy;
    return vout;
}