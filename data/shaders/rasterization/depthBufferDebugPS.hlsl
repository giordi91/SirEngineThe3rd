#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"



Texture2D sourceTexture: register(t0);
ConstantBuffer<DebugLayerConfig> g_debugConfig: register(b1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(FullScreenVertexOut pin) : SV_Target
{

	float DepthMaxRange = g_debugConfig.depthMax;
	float DepthMinRange = g_debugConfig.depthMin;
    float depth = sourceTexture.Sample(gsamPointClamp, pin.uv).x;
	//remapping depth
	float range = DepthMaxRange - DepthMinRange;
	float delta = depth - DepthMinRange;
	delta = delta < 0 ? range : delta; 
	depth = saturate(delta / abs(DepthMaxRange - DepthMinRange));
	return float4(depth, 0, 0, 1);
}
