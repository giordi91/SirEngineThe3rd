#include "../common/camera.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


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
    float4 Normal: Normal;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(vin.PosL, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= vin.Normal;
    return vout;
}