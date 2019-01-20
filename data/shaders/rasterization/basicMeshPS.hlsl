struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Normal: NORMAL;
};

float4 PS(VertexOut pin) : SV_Target
{
    return float4(1,0,0,0);
}


