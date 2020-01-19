#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"


ConstantBuffer<CameraBuffer> g_camera: register(b0,space0);
//ConstantBuffer<DebugPointsFixedColor> g_settings: register(b1,space1);


float4 PS(PositionOnlyVertexOut pin) : SV_Target
{
    float4 color = float4(0, 0.8, 0, 1);
    return color;
}


