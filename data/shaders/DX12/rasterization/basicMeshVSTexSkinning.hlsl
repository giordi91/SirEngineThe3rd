#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
StructuredBuffer<int> g_influences : register(t5,space3);
StructuredBuffer<float> g_weights : register(t6,space3);
StructuredBuffer<float4x4> g_matrices : register(t7,space3);
StructuredBuffer<float4> g_positions: register(t8,space3);
StructuredBuffer<float4> g_normals: register(t9,space3);
StructuredBuffer<float2> g_uvs: register(t10,space3);
StructuredBuffer<float4> g_tangents: register(t11,space3);

#include "../common/skinning.hlsl"

FullMeshVertexOut VS(uint vid : SV_VertexID)
{
    FullMeshVertexOut vout;
    float4 pos = g_positions[vid];
	FullSkinResult skin = skinFullPoint(vid,pos,g_normals[vid].xyz,g_tangents[vid].xyz);
	
	// Transform to homogeneous clip space.
    //vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
    vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = normalize(skin.normal);
    vout.uv = g_uvs[vid];
    vout.tangent = normalize(skin.tan);
    vout.worldPos = pos;
    return vout;
}
