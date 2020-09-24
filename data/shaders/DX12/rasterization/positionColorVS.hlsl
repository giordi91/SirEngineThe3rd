#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<FrameData> g_frameData: register(b0,space0);
StructuredBuffer<float4> g_positions: register(t0, space3);

PositionColor VS(uint vid : SV_VertexID)
{
	PositionColor vout;
	
	// Transform to homogeneous clip space.
    vout.position = mul(g_positions[vid*2], g_frameData.m_activeCamera.MVP);
	vout.color = g_positions[vid*2+1];

    return vout;
}