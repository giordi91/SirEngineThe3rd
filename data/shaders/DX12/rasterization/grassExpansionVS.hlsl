#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<FrameData> g_frameData : register(b0, space0);
ConstantBuffer<GrassConfig> grassConfig: register(b3, space3);
ByteAddressBuffer points : register(t0, space3);
ByteAddressBuffer tileIndices: register(t1, space3);
ByteAddressBuffer tilesCulling: register(t5, space3);
ByteAddressBuffer lodToUse: register(t6, space3);
Texture2D windTex : register(t2, space3);


SamplerState gsamPointWrap : register(s0,space1);
SamplerState gsamPointClamp : register(s1,space1);
SamplerState gsamLinearWrap : register(s2,space1);
SamplerState gsamLinearClamp : register(s3,space1);
SamplerState gsamAnisotropicWrap : register(s4,space1);
SamplerState gsamAnisotropicClamp : register(s5,space1);


static const float2 offsets[15] = {
    float2(-0.5f, 0.0f), 
    float2(0.5f, 0.0f), 
    float2(-0.5f, 0.33333f), 

    float2(0.5f, 0.0f), 
    float2(0.5f, 0.333333f), 
    float2(-0.5f, 0.33333f), 

    float2(-0.5f, 0.33333f), 
    float2(0.5f, 0.333333f), 
    float2(-0.5f, 0.66666f), 

    float2(0.5f, 0.333333f), 
    float2(0.5f, 0.66666), 
    float2(-0.5f, 0.66666), 

    float2(-0.5f, 0.66666), 
    float2(0.5f, 0.66666), 
    float2(0.0f, 1.0f) 
};

static const float2 bladeUVs[15] = {
    float2(0.0f, 0.0f), 
    float2(1.0f, 0.0f), 
    float2(0.0f, 0.333333f), 

    float2(1.0f, 0.0f), 
    float2(1.0f, 0.333333f), 
    float2(0.0f, 0.333333f), 

    float2(0.0f, 0.33f), 
    float2(1.0f, 0.33), 
    float2(0.0f, 0.66666f), 

    float2(1.0f, 0.33f), 
    float2(1.0f, 0.666666), 
    float2(0.0f, 0.666666), 

    float2(0.0f, 0.6666f), 
    float2(1.0f, 0.6666f), 
    float2(0.5f, 1.0), 

};

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0*3.14159265359;
static const float3 taper = float3(0.75f, 0.65f,0.45f);

// Simple noise function, sourced from http://answers.unity.com/answers/624136/view.html
	// Extended discussion on this function can be found at the following link:
	// https://forum.unity.com/threads/am-i-over-complicating-this-random-function.454887/#post-2949326
	// Returns a number in the 0...1 range.
	float rand(float3 co)
	{
		return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
	}

    // Construct a rotation matrix that rotates around the provided axis, sourced from:
	// https://gist.github.com/keijiro/ee439d5e7388f3aafc5296005c8c3f33
	float3x3 angleAxis3x3(float angle, float3 axis)
	{
		float c = cos(angle);
        float s = sin(angle);

		float t = 1 - c;
		float x = axis.x;
		float y = axis.y;
		float z = axis.z;

		return float3x3(
			t * x * x + c, t * x * y - s * z, t * x * z + s * y,
			t * x * y + s * z, t * y * y + c, t * y * z - s * x,
			t * x * z - s * y, t * y * z + s * x, t * z * z + c
			);
	}

struct GrassCullingResult
{
    int tileIndex; 
	uint16_t tilePointsId; 
	uint16_t LOD; 
};

PosNormalUVVertexOut VS(uint id : SV_VertexID)
{
    PosNormalUVVertexOut vout;
    uint vid = id/15;
	uint localId = id%15;

	float width = grassConfig.width;
	float height = grassConfig.height;
	float widthRandom = grassConfig.widthRandom;
	float heightRandom = grassConfig.heightRandom;

    int inLod = int(lodToUse.Load<uint>(0));
    int totalTiles = grassConfig.tilesPerSide*grassConfig.tilesPerSide;
    int lodOffset = totalTiles *inLod ;
    //int tileNumber = int(vid/grassConfig.pointsPerTile);
    int pointInTile = int(grassConfig.pointsPerTileLod[inLod]);
    int cullIndex = int(vid/pointInTile);
    GrassCullingResult cullTileData;// = cullTileId[cullIndex+SUPPORT_DATA_OFFSET + lodOffset];

    cullTileData.tileIndex = tilesCulling.Load<int>(((cullIndex + 40) + lodOffset) * 8 + 0); //value at offset 0
    //we have two uint16 here so we shift by 2 bytes 
    cullTileData.tilePointsId = tilesCulling.Load<uint16_t>(((cullIndex + 40) + lodOffset) * 8 + 4);
    cullTileData.LOD = tilesCulling.Load<uint16_t>(((cullIndex + 40) + lodOffset) * 8 + 6);
    int tileNumber = int(cullTileData.tileIndex);


    int tilesPerSide = grassConfig.tilesPerSide;
    float halfSize = tilesPerSide*0.5;
    float tw = grassConfig.tileSize;

    float3 minCorner = grassConfig.gridOrigin - float3(halfSize,0,halfSize)*tw;
    float tileX = tileNumber %tilesPerSide;
    float tileY = tileNumber /tilesPerSide;
    float3 tileCorner = minCorner + float3(tw*(tileX), 0, tw*tileY);

    int tempOffset = cullTileData.tilePointsId *grassConfig.pointsPerSourceTile;
    int inTilePosIdx = int(vid%pointInTile);
	float2 tilePos = points.Load<float2>((inTilePosIdx + tempOffset) * 8 + 0);;
    float3 position = tileCorner + float3(tilePos.x,0.0f,tilePos.y)*tw;
    float4 offset = float4(offsets[localId], 0.0f, 0.0f);


    //spinning the blade by random amount
    float3x3 facingRotationMatrix = angleAxis3x3(rand(position.xyz) * TWO_PI, float3(0, 1, 0));

    //wind computation
    float range = halfSize*tw; 
    float2 uv = bladeUVs[localId];

	float2 tempUV = float2((position.x / range)*0.5 + 1,(position.z / range)*0.5 + 1)*0.2 ;
    float2 windUV = frac( tempUV+ grassConfig.windFrequency* g_frameData.time);
    //float2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windUV).xy*2 -1  );
	//float3 wind = normalize(float3(windSample.x, 0, windSample.y));
    //uint notCulled = cullTileId[tileNumber];
    float windOffsetHeight = 0.15;
    float2 windOffset = -windOffsetHeight*uv.y *grassConfig.windFrequency + windUV;
    //float2 windSample = (texture (sampler2D (windTex, [2]), windOffset).xy*2 -1  );
	float2 windSample =(windTex.SampleLevel(gsamLinearClamp, windOffset,0).xy*2 -1);
	float3 wind = normalize(float3(windSample.x, 0, windSample.y));
    float3x3 windRotation = angleAxis3x3(lerp(-PI*0.5*grassConfig.windStrength,PI*0.5*grassConfig.windStrength, length(windSample)), wind);

    //bending the blade
    float3x3 bendRotationMatrix = angleAxis3x3(rand(position.zzx) * grassConfig.grassBend* PI * 0.5, float3(1, 0, 0));


    //computing the height and width for the blade
    float finalHeight = (rand(position.zyx) * 2.0 - 1.0) * heightRandom+ height;
	float finalWidth = (rand(position.xzy) * 2.0 - 1.0) * widthRandom + width;

    //computing forward
	float forward= rand(position.yyz) * grassConfig.bladeForward;
    float segmentForward = pow(uv.y, grassConfig.bladeCurvatureAmount) * forward;

    //performing taper of the blade
	float a = taper.x * float(uv.y < 0.33f);
    float b = taper.y * float(int (uv.y <= 0.66) & int(uv.y >= 0.33));
	float c = taper.z * float(uv.y > 0.66);

    float taperValue  = a+ b + c;

    //let us compute the triangle we are in
    uint triangleId = localId/6;
    float  localWidth =  finalWidth* taperValue;

    offset.x *=localWidth;
    offset.y *=finalHeight;
    offset.z  += segmentForward;
		
    float3x3 transformation =mul(mul(bendRotationMatrix,facingRotationMatrix),windRotation); 
    float3 rotatedOffset = mul(offset.xyz,transformation);
    position += rotatedOffset;

    vout.worldPos = float4(position,1.0);
    vout.PosH  = mul(float4(position,1.0),  g_frameData.m_activeCamera.MVP) ;
    vout.Normal = normalize(float4(mul(float3(0,segmentForward,-1),transformation),0));
    vout.uv = uv;
	
    return vout;
}