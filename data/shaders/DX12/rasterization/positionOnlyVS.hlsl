#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

PositionOnlyVertexOut VS(PositionOnlyVertexIn  vin)
{
	PositionOnlyVertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.pos = mul(float4(vin.pos,1.0f) , g_cameraBuffer.MVP);
	
    return vout;
}