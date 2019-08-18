#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"


ConstantBuffer<CameraBuffer> g_camera: register(b1);
ConstantBuffer<DebugPointsFixedColor> g_settings: register(b1);


float4 PS(PositionOnlyVertexOut pin) : SV_Target
{
    //return float4(g_settings.color);
    return float4(1.0f,0.0f,0.0f,1.0f);
}


