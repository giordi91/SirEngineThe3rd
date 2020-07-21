#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
StructuredBuffer<float4> g_positions : register(t0,space3);
StructuredBuffer<float4> g_normals : register(t1,space3);
StructuredBuffer<float2> g_uvs : register(t2,space3);
StructuredBuffer<float4> g_tangents : register(t3,space3);


FullMeshVertexOut VS( uint vid : SV_VertexID)
{
	FullMeshVertexOut vout;
	
    float4 p = g_positions[vid];
	// Transform to homogeneous clip space.
	vout.PosH = mul(p, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= g_normals[vid].xyz;
	vout.uv = g_uvs[vid];
	//vout.tangent = g_tangents[vid].xyz;
    vout.tangent = g_tangents[vid].xyz;
    vout.worldPos = p;
    return vout;
}