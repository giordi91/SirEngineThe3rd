#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
StructuredBuffer<int> g_influences : register(t5);
StructuredBuffer<float> g_weights : register(t6);
StructuredBuffer<float4x4> g_matrices : register(t7);

#include "../common/skinning.hlsl"

FullMeshVertexOut VS(TexturedVertexIn12 vin, uint vid : SV_VertexID)
{
    FullMeshVertexOut vout;
	FullSkinResult skin = skinFullPoint(vid,vin);
	
	// Transform to homogeneous clip space.
    //vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
    vout.PosH = mul(skin.pos, g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = normalize(skin.normal);
    vout.uv = vin.uvs.xy;
    vout.tangent = normalize(skin.tan);
    vout.worldPos = float4(vin.PosL, 1.0f);
    return vout;
}