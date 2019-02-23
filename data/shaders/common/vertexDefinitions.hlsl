#ifndef VTX_DEFINITIONS_HLSL 
#define VTX_DEFINITIONS_HLSL

struct TexturedVertexIn16
{
	float4 PosL  : POSITION;
    float4 Normal : NORMAL;
    float4 uvs: TEXCOORD;
    float4 tangents: TANGENTS;
};
struct TexturedVertexIn12
{
	float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 uvs: TEXCOORD;
    float4 tangents: TANGENTS;
};

struct PosNormalVertexOut 
{
	float4 PosH  : SV_POSITION;
    float4 Normal: Normal;
};


struct FullMeshVertexOut
{
	float4 PosH  : SV_POSITION;
	float2 uv :TEXCOORD;
    float4 Normal: NORMAL;
    float4 tangent: TANGENTS;
};

struct FullScreenVertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

#endif
