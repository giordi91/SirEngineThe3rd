#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0, space0);
ConstantBuffer<DebugPointsFixedColor> g_settings : register(b1, space3);
StructuredBuffer<float4> points : register(t0, space3);

static const float width = 0.2;
static const float height = 0.4;
static float4 offsets[] =
{
    { 1.0f, 0.0f, 0.0f, width },
    { 0.0f, 1.0f, 0.0f, height },
    { -1.0f, 0.0f, 0.0f, width },
};

PositionOnlyVertexOut VS(uint id : SV_VertexID)
{
    PositionOnlyVertexOut vout;
    uint vid = id/3;
	uint localId = id%3;
	float4 position = points[vid];
	float4 offset = offsets[localId];
    //scaling the offset vector
	offset*=offset.w;
	offset.w=0.0f;

    vout.worldPos = offset + points[id / 6];
    vout.pos  = mul(offset + points[id / 6],  g_cameraBuffer.ViewMatrix) ;
    /*
    float3 offset = offsets[id % 6] * g_settings.pointSize;

    float3 up = { 0, 1, 0 };
    float3 view = float3(g_cameraBuffer.ViewMatrix._31, g_cameraBuffer.
    ViewMatrix._31, g_cameraBuffer.ViewMatrix._31);

    offset = mul(offset, (float3x3) g_cameraBuffer.ViewMatrix) + points[id / 6].xyz;

	// Transform to homogeneous clip space.
    vout.worldPos = float4(offset, 1.0);
    vout.pos = mul(vout.worldPos, g_cameraBuffer.MVP);
    */
	
    return vout;
}
