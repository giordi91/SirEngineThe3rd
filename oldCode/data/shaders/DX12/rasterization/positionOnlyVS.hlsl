#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
StructuredBuffer<float4> g_positions: register(t0, space3);

PositionOnlyVertexOut VS(uint vid : SV_VertexID)
{
	PositionOnlyVertexOut vout;
	
	// Transform to homogeneous clip space.
    vout.pos = mul(g_positions[vid], g_cameraBuffer.MVP);
	
    return vout;
}