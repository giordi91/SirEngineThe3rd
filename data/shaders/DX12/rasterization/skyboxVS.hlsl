#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

PositionOnlyVertexOut VS(TexturedVertexIn12 vin)
{
    PositionOnlyVertexOut vout;
	
    //offsetting  the position such that the skybox is always on the camera
    float4 offsetPos = float4(vin.PosL, 0.0f) + g_cameraBuffer.position;
    offsetPos.w = 1.0f;
    vout.pos = mul(offsetPos, g_cameraBuffer.MVP);
    vout.worldPos = float4(vin.PosL, 1.0f);
    return vout;
}