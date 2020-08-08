#include "../common/vertexDefinitions.hlsl"
#include "../common/structures.hlsl"

ConstantBuffer<CameraBuffer> g_camera: register(b0,space0);
ConstantBuffer<GrassConfig> grassConfig: register(b3,space3);

static const float4 arrBasePos[6] = {
    float4(-1.0f, -1.0f, 0.0f, 1.0f), 
	float4(1.0f, 1.0f, 1.0f, 0.0f),
    float4(-1.0f, 1.0f, 0.0f, 0.0f), 
	float4(1.0f, 1.0f, 1.0f, 0.0f),
	float4(-1.0f, -1.0f, 0.0f, 1.0f),
	float4(1.0f, -1.0f, 1.0f, 1.0f)
	};

FullScreenVertexOut VS( uint vid : SV_VertexID) {
  FullScreenVertexOut vout;
  float4 p = arrBasePos[vid];

  int tilesPerSide = grassConfig.tilesPerSide;
  float halfSize = tilesPerSide*0.5;
  float tw = grassConfig.tileSize;
  float3 localPos = float3(p.x,0.0f,p.y);
  float3 position = grassConfig.gridOrigin + ((halfSize*tw)*localPos);
  
  vout.pos = mul(float4( position,1.0f),g_camera.MVP);
  vout.uv = p.zw;
  return vout;
}
