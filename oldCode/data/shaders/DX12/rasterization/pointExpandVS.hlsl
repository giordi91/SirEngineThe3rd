#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
ConstantBuffer<DebugPointsFixedColor> g_settings : register(b1,space3);
StructuredBuffer<float4> points : register(t0,space3);

static float3 offsets[] =
{
    { -1.0f, -1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f }
};

PositionOnlyVertexOut VS(uint id : SV_VertexID)
{
    PositionOnlyVertexOut vout;
    float3 offset = offsets[id % 6] * g_settings.pointSize;

    float3 up = { 0, 1, 0 };
    float3 view = float3(g_cameraBuffer.ViewMatrix._31, g_cameraBuffer.
    ViewMatrix._31, g_cameraBuffer.ViewMatrix._31);

    offset = mul(offset, (float3x3)g_cameraBuffer.ViewMatrix) + points[id/6].xyz;

	// Transform to homogeneous clip space.
    vout.worldPos = float4(offset, 1.0);
    vout.pos = mul(vout.worldPos, g_cameraBuffer.MVP);
	
    return vout;
}