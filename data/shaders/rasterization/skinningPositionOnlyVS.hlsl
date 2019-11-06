#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
StructuredBuffer<int> g_influences : register(t5);
StructuredBuffer<float> g_weights : register(t6);
StructuredBuffer<float4x4> g_matrices : register(t7);
static const int NUMBER_OF_INFLUENCES = 6;


PositionOnlyVertexIn VS(TexturedVertexIn12 vin, uint vid : SV_VertexID)
{
    PositionOnlyVertexIn vout;
    int id = vid* NUMBER_OF_INFLUENCES;

    float4 skin_p = float4(0, 0, 0, 1);
    float4 v = float4(vin.PosL, 1.0f);

    float4 temp;
    float3 tempVec3;
    for (int i = 0; i < NUMBER_OF_INFLUENCES; ++i)
    {
        temp = mul(v, g_matrices[g_influences[id + i]]);
        skin_p += (g_weights[id + i] * temp);
    }
    skin_p.w = 1.0f;
	
	// Transform to homogeneous clip space.
    vout.pos = mul(skin_p, g_cameraBuffer.MVP).xyz;
    return vout;
}