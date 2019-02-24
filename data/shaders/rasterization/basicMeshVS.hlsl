#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

PosNormalVertexOut VS(TexturedVertexIn16 vin)
{
	PosNormalVertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(vin.PosL, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= vin.Normal;
    return vout;
}