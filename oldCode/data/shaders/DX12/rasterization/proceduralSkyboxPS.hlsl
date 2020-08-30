
#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);


float4 PS(FullScreenVertexOut pin) : SV_Target
{
    float worldDir = mul(float4(pin.clipPos, 0.0f, 1.0f), g_cameraBuffer.VPinverse).y;
    //float worldDir = dot(float4(pin.clipPos, 0.0f, 1.0f), transpose(g_cameraBuffer.VPinverse)[1]);
    //float worldDir = dot(float3(pin.clipPos, 0.0f), float3(g_cameraBuffer.VPinverse[0].y,
    //g_cameraBuffer.VPinverse[1].y, g_cameraBuffer.VPinverse[2].y) );

	//old
    //float3 ground = float3(0.412f, 0.380, 0.357);
    //float3 skyTop = float3(0.357f, 0.451f, 0.6f);
    //float3 skybottom = float3(0.906f, 1.0f, 1.0f);

	//degammad color
    float3 ground = float3(0.142f, 0.119, 0.103);
    float3 skyTop = float3(0.103f, 0.173f, 0.325f);
    float3 skybottom = float3(0.804f, 1.0f, 1.0f);

    float bottomGradientDiffusion = 15.0f;
    float topGradientDiffiusion= 2.0f;
    float verticalGradient = worldDir;
    float topLerpFactor = saturate(verticalGradient*topGradientDiffiusion);
    float bottomLerpFactor = saturate(-verticalGradient*bottomGradientDiffusion);

    //sky
    if(worldDir > 0)
    {
        float4 color = float4(lerp(skybottom, skyTop, topLerpFactor), 1.0f);
        return color;
    }
    //ground
    else
    {
        float4 color = float4(lerp(skybottom, ground, bottomLerpFactor), 1.0f);
        return color;
    }
}

 