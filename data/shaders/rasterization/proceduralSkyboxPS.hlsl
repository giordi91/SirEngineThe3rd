
#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


float4 PS(FullScreenVertexOut pin) : SV_Target
{
    float4 worldDir = normalize(mul(float4(pin.clipPos, 0.0f, 1.0f), g_cameraBuffer.VPinverse));

    float3 ground = float3(0.412f, 0.380, 0.357);
    float3 skyTop = float3(0.357f, 0.451f, 0.6f);
    float3 skybottom = float3(0.906f, 1.0f, 1.0f);
    if(worldDir.y > 0)
    {
        float fractional = frac(worldDir.y);
        float4 color = float4(lerp(skybottom, skyTop, fractional), 1.0f);
    return color;
    }
    else
    {
        float4 color = float4(ground, 1.0f);
    return color;
    }
}

 