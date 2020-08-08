#ifndef VTX_DEFINITIONS_HLSL 
#define VTX_DEFINITIONS_HLSL

struct PositionOnlyVertexIn 
{
	float3 pos : POSITION;
};

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
struct PosNormalUVVertexOut 
{
	float4 PosH  : SV_POSITION;
    float4 Normal: Normal;
	float4 worldPos: POSITION;
    float2 uv : TEXCOORD;
};


struct FullMeshVertexOut
{
	float4 PosH  : SV_POSITION;
	float4 worldPos: POSITION;
	float2 uv :TEXCOORD;
    float3 Normal: NORMAL;
    float3 tangent: TANGENTS;
};

struct FullMeshParallaxVertexOut
{
	float4 PosH  : SV_POSITION;
	float4 worldPos: POSITION;
    float2 uv : TEXCOORD0;
    float3 Normal: NORMAL;
    float3 tangent: TANGENTS;
    float4 tangentFragPos: TEXCOORD1;
    float4 tangentViewPos: TEXCOORD2;
};

struct FullScreenVertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

struct PositionOnlyVertexOut{
  float4 pos : SV_POSITION;
  float4 worldPos: POSITION;
};
struct LocalPositionOnlyVertexOut{
  float4 pos : SV_POSITION;
};

struct PositionColor
{
	float4 position: SV_POSITION ;
	float4 color: TEXCOORD1;
};

#endif
