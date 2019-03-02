struct VertexOut {
  float4 pos : SV_POSITION;
  float2 clipPos : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = float4(1.0f, 1.0f, 0.0f, 1.0f);
    return color;
}


