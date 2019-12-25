#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


FullMeshVertexOut VS( uint vid : SV_VertexID)
{
	FullMeshVertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL,1.0f), g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= vin.Normal;
	vout.uv = vin.uvs.xy;
	vout.tangent = vin.tangents.xyz;
	vout.worldPos = float4(vin.PosL,1.0f);
    return vout;
}