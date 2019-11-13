#ifndef SKINNING_HLSL
#define SKINNING_HLSL

static const int NUMBER_OF_INFLUENCES = 6;

struct FullSkinResult {
	float4 pos;
	float3 normal;
	float3 tan;
};

FullSkinResult skinFullPoint(int vid, TexturedVertexIn12 vin) {

  FullSkinResult result;
  int id = vid * NUMBER_OF_INFLUENCES;

  float4 skinPos = float4(0, 0, 0, 1);
  float3 skinNormal = float3(0, 0, 0);
  float3 skinTan = float3(0, 0, 0);
  float4 v = float4(vin.PosL, 1.0f);

  float4 temp;
  float3 tempVec3;
  for (int i = 0; i < NUMBER_OF_INFLUENCES; ++i) {
    temp = mul(v, g_matrices[g_influences[id + i]]);
    skinPos += (g_weights[id + i] * temp);
    tempVec3 = mul(vin.Normal, (float3x3)g_matrices[g_influences[id + i]]);
    skinNormal += (g_weights[id + i] * tempVec3);
    tempVec3 =
        mul(vin.tangents.xyz, (float3x3)g_matrices[g_influences[id + i]]);
    skinTan += (g_weights[id + i] * tempVec3);
  }
  skinPos.w = 1.0f;
  result.pos = skinPos;
  result.normal = skinNormal;
  result.tan = skinTan;
  return result;
}

#endif
