
struct TexturedVertexIn 
{
	float4 PosL  : POSITION;
    float4 Normal : NORMAL;
    float4 uvs: TEXCOORD;
    float4 tangents: TANGENTS;
};

struct TexturedVertexOut 
{
	float4 PosH  : SV_POSITION;
    float4 Normal: Normal;
};
