#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<FrameData> g_frameData : register(b0,space0);
ByteAddressBuffer vertices: register(t0,space3);
ByteAddressBuffer normals: register(t1,space3);
ByteAddressBuffer uvs: register(t2,space3);
ByteAddressBuffer tangents: register(t3,space3);


FullMeshVertexOut VS( uint vid : SV_VertexID)
{
	FullMeshVertexOut vout;
	
    float4 p = vertices.Load<float4>(vid * 16);
	// Transform to homogeneous clip space.
	vout.PosH = mul(p, g_frameData.m_activeCamera.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= normals.Load<float4>(vid*16).xyz;
	vout.uv = uvs.Load<float2>(vid*8);
	//vout.tangent = g_tangents[vid].xyz;
    vout.tangent = tangents.Load<float4>(vid*16).xyz;
    vout.worldPos = p;
    return vout;
}