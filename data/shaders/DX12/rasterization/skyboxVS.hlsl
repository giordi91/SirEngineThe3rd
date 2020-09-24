#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<FrameData> g_frameData : register(b0,space0);

float3 createCube( uint vertexID)
{
	uint b = 1 << vertexID;
	float x = (0x287a & b) != 0;
	float y = (0x02af & b) != 0;
	float z = (0x31e3 & b) != 0;
	return float3(x,y,z);
}

PositionOnlyVertexOut VS(uint vid : SV_VertexID)
{
    PositionOnlyVertexOut vout;
	
    float4 pos = float4(createCube(vid) - float3(0.5,0.5,0.5),1.0);
    //offsetting  the position such that the skybox is always on the camera
    float4 offsetPos = pos +g_frameData.m_activeCamera.position;
    offsetPos.w = 1.0f;
    vout.pos = mul(offsetPos, g_frameData.m_activeCamera.MVP);
    vout.worldPos = pos;
    return vout;
}