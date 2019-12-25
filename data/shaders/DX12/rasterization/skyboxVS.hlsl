#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
StructuredBuffer<float4> g_positions: register(t8);

PositionOnlyVertexOut VS(uint vid : SV_VertexID)
{
    PositionOnlyVertexOut vout;
	
    float4 pos = g_positions[vid];
    //offsetting  the position such that the skybox is always on the camera
    float4 offsetPos = pos + g_cameraBuffer.position;
    offsetPos.w = 1.0f;
    vout.pos = mul(offsetPos, g_cameraBuffer.MVP);
    vout.worldPos = pos;
    return vout;
}