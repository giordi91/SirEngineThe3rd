#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

PositionOnly VS(TexturedVertexIn12 vin)
{
	PositionOnly vout;
	
	// Transform to homogeneous clip space.
	
	vout.pos = mul(float4(vin.PosL,1.0f), g_cameraBuffer.MVP);
	vout.worldPos= float4(vin.PosL,1.0f);
    return vout;
}