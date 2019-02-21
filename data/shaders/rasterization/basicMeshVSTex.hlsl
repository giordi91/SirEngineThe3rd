#include "../common/camera.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


FullMeshVertexOut VS(TexturedVertexIn12 vin)
{
	FullMeshVertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL,1.0f), g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal= float4(vin.Normal,0.0f);
	vout.uv = vin.uvs.xy;
    return vout;
}