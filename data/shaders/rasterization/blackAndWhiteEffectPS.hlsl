struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};
float4 PS(VertexOut pin) : SV_Target
{
    return float4(1,0,0,0);
}


