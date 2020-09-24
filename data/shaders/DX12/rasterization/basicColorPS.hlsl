#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"


//ConstantBuffer<CameraBuffer> g_camera: register(b0,space0);
//ConstantBuffer<DebugPointsFixedColor> g_settings: register(b1,space3);


float4 PS(PositionColor pin) : SV_Target
{
    return float4(pin.color);
}


