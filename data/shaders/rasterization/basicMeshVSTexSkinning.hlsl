#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
StructuredBuffer<int> g_influences : register(t4);
StructuredBuffer<float> g_weights : register(t5);
StructuredBuffer<float4x4> g_matrices : register(t6);
static const int NUMBER_OF_INFLUENCES = 6;


FullMeshVertexOut VS(TexturedVertexIn12 vin, uint vid : SV_VertexID)
{
    FullMeshVertexOut vout;
    int id = vid* NUMBER_OF_INFLUENCES;

    float4 skin_p = float4(0, 0, 0, 1);
    float4 v = float4(vin.PosL, 1.0f);

    float4 temp;
    for (int i = 0; i < NUMBER_OF_INFLUENCES; ++i)
    {
        temp = mul(v, g_matrices[g_influences[id + i]]);
        skin_p += (g_weights[id + i] * temp);
    }
    skin_p.w = 1.0f;
	
	// Transform to homogeneous clip space.
    vout.PosH = mul(skin_p, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = vin.Normal;
    vout.uv = vin.uvs.xy;
    vout.tangent = vin.tangents.xyz;
    vout.worldPos = float4(vin.PosL, 1.0f);
    return vout;
}