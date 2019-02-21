#include "../common/camera.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

struct VertexIn
{
	float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 uvs: TEXCOORD;
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
	vout.PosH = mul(float4(vin.PosL,1.0f), g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= float4(vin.Normal,0.0f);
	vout.uv = vin.uvs.xy;
    return vout;
}