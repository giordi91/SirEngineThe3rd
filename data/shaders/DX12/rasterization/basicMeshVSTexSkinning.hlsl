#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
StructuredBuffer<int> g_influences : register(t5);
StructuredBuffer<float> g_weights : register(t6);
StructuredBuffer<float4x4> g_matrices : register(t7);
StructuredBuffer<float4> g_positions: register(t8);
StructuredBuffer<float4> g_normals: register(t9);
StructuredBuffer<float2> g_uvs: register(t10);
StructuredBuffer<float4> g_tangents: register(t11);

#include "../common/skinning.hlsl"

FullMeshVertexOut VS(TexturedVertexIn12 vin, uint vid : SV_VertexID)
{
    FullMeshVertexOut vout;
	FullSkinResult skin = skinFullPoint(vid,g_positions[vid],g_normals[vid].xyz,g_tangents[vid].xyz);
	
	// Transform to homogeneous clip space.
    //vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
    vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = normalize(skin.normal);
    vout.uv = g_uvs[vid];
    vout.tangent = normalize(skin.tan);
    vout.worldPos = float4(vin.PosL, 1.0f);
    return vout;

/*
    float4 p = g_positions[vid];
    vout.PosH = mul(p, g_cameraBuffer.MVP);
    vout.Normal = normalize(g_normals[vid]);
    vout.uv= g_uvs[vid];
    vout.tangent= g_tangents[vid];
    vout.worldPos = p;
    return vout;
*/


    /*
    float4 p = g_positions[0];
    vout.PosH = mul(p, g_cameraBuffer.MVP);
    vout.Normal = float3(0, 0, 0);
    vout.uv = float2(0, 0);
    vout.tangent = float3(0, 0, 0);
    vout.worldPos = p;
    return vout;
    */
}
