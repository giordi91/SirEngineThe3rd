#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
StructuredBuffer<float4> g_positions : register(t9,space3);
StructuredBuffer<float4> g_normals : register(t10,space3);
StructuredBuffer<float2> g_uvs : register(t11,space3);
StructuredBuffer<float4> g_tangents : register(t12,space3);

FullMeshParallaxVertexOut VS(uint vid : SV_VertexID)
{
    FullMeshParallaxVertexOut vout;
	
    float4 p = g_positions[vid];
	// Transform to homogeneous clip space.
    vout.PosH = mul(p, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = g_normals[vid].xyz;
    vout.uv = g_uvs[vid];
    vout.tangent = g_tangents[vid];
    vout.worldPos = p;

    float3 N = normalize(vout.Normal);
    float3 T = normalize(vout.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 NTB = transpose(float3x3(T, B, N));

    //extra data needed for compute view direction in tangent space
    //TODO we already have all the necessary data, normal, tangent etc in the frag
    //maybe we can simply built the matrix there and convert to tangent space?
    //might be more multiplications but less memory moving around from the raster
    vout.tangentFragPos = float4(mul(vout.worldPos.xyz, NTB), 0.0);
    vout.tangentViewPos = float4(mul(g_cameraBuffer.position.xyz, NTB), 0.0);

    return vout;
}