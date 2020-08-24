#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/constants.glsl"
#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 

layout (set=3,binding=0) buffer vertices
{
	vec2 p[];
};
layout (set=3,binding=1) buffer tilesIndices 
{
	uint tileId[];
};

layout (set=3,binding = 2) uniform texture2D windTex;
layout (set=3,binding=3) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 
layout (set=3,binding=5) buffer tilesCulling
{
	ivec2 cullTileId[];
};

layout (set=1,binding = 0) uniform sampler[7] colorSampler;


const vec3 offsets[15] = {
    vec3(-0.5f, 0.0f, 0.0f), 
    vec3(0.5f, 0.0f, 0.0f), 
    vec3(-0.5f, 0.33333f, 0.0f), 

    vec3(0.5f, 0.0f, 0.0f), 
    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(-0.5f, 0.33333f, 0.0f), 

    vec3(-0.5f, 0.33333f, 0.0f), 
    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(-0.5f, 0.66666f, 0.0f), 

    vec3(0.5f, 0.333333f, 0.0f), 
    vec3(0.5f, 0.66666, 0.0f), 
    vec3(-0.5f, 0.66666, 0.0f), 

    vec3(-0.5f, 0.66666, 0.0f), 
    vec3(0.5f, 0.66666, 0.0f), 
    vec3(0.0f, 1.0f, 0.0f) 
};

const vec2 bladeUVs[15] = {
    vec2(0.0f, 0.0f), 
    vec2(1.0f, 0.0f), 
    vec2(0.0f, 0.333333f), 

    vec2(1.0f, 0.0f), 
    vec2(1.0f, 0.333333f), 
    vec2(0.0f, 0.333333f), 

    vec2(0.0f, 0.33f), 
    vec2(1.0f, 0.33), 
    vec2(0.0f, 0.66666f), 

    vec2(1.0f, 0.33f), 
    vec2(1.0f, 0.666666), 
    vec2(0.0f, 0.666666), 

    vec2(0.0f, 0.6666f), 
    vec2(1.0f, 0.6666f), 
    vec2(0.5f, 1.0), 

};

const vec3 taper = vec3(0.75f, 0.65f,0.45f);


/*
//to test
float whangHashNoise(uint u, uint v, uint s)
{
    //return fract(sin(float(s + (u*1080u + v)%10000u) * 78.233) * 43758.5453);
    
    uint seed = (u*1664525u + v) + s;
    
    seed  = (seed ^ 61u) ^(seed >> 16u);
    seed *= 9u;
    seed  = seed ^(seed >> 4u);
    seed *= uint(0x27d4eb2d);
    seed  = seed ^(seed >> 15u);
    
    float value = float(seed) *(1.0/4294967296.0);
    return value;
}
*/




// Simple noise function, sourced from http://answers.unity.com/answers/624136/view.html
	// Extended discussion on this function can be found at the following link:
	// https://forum.unity.com/threads/am-i-over-complicating-this-random-function.454887/#post-2949326
	// Returns a number in the 0...1 range.
	float rand(vec3 co)
	{
		return fract(sin(dot(co.xyz, vec3(12.9898, 78.233, 53.539))) * 43758.5453);
	}

    // Construct a rotation matrix that rotates around the provided axis, sourced from:
	// https://gist.github.com/keijiro/ee439d5e7388f3aafc5296005c8c3f33
	mat3x3 angleAxis3x3(float angle, vec3 axis)
	{
		float c = cos(angle);
        float s = sin(angle);

		float t = 1 - c;
		float x = axis.x;
		float y = axis.y;
		float z = axis.z;

		return mat3x3(
			t * x * x + c, t * x * y - s * z, t * x * z + s * y,
			t * x * y + s * z, t * y * y + c, t * y * z - s * x,
			t * x * z - s * y, t * y * z + s * x, t * z * z + c
			);
	}


const int SUPPORT_DATA_OFFSET = 16;


layout(location =1) out vec2 outUV;
layout(location =2) out vec3 outNormal;
layout (location = 3) out vec3 worldPos;

void VS()
{

    uint vid = gl_VertexIndex/15;
	uint localId = gl_VertexIndex%15;

    //grass width/height
	float width = grassConfig.width;
	float height = grassConfig.height;
	float widthRandom = grassConfig.widthRandom;
	float heightRandom = grassConfig.heightRandom;

    //int tileNumber = int(vid/grassConfig.pointsPerTile);
    int cullIndex = int(vid/grassConfig.pointsPerTile);
    ivec2 cullTileData = cullTileId[cullIndex+SUPPORT_DATA_OFFSET];
    int tileNumber = int(cullTileData.x);

    //uint notCulled = cullTileId[tileNumber];

    int tilesPerSide = grassConfig.tilesPerSide;
    float halfSize = tilesPerSide*0.5;
    float tw = grassConfig.tileSize;

    vec3 minCorner = grassConfig.gridOrigin - vec3(halfSize,0,halfSize)*tw;
    float tileX = tileNumber %tilesPerSide;
    float tileY = tileNumber /tilesPerSide;
    vec3 tileCorner = minCorner + vec3(tw*(tileX), 0, tw*tileY);

    int tempOffset = cullTileData.y *grassConfig.pointsPerTile ;
    int inTilePosIdx = int(vid%grassConfig.pointsPerTile);
	vec2 tilePos = p[inTilePosIdx + tempOffset];
    vec3 position = tileCorner + vec3(tilePos.x,0.0f,tilePos.y)*tw;
	vec4 offset = vec4(offsets[localId],0.0f);



    //spinning the blade by random amount
    mat3x3 facingRotationMatrix = angleAxis3x3(rand(position.xyz) * TWO_PI, vec3(0, 1, 0));

    vec2 uv = bladeUVs[localId];

    //sampling the wind texture
    float range = halfSize*tw; 

    /*
	vec2 tempUV = vec2((position.x / range)*0.5 + 1,(position.z / range)*0.5 + 1) * 0.1;
    vec2 windUV = fract( tempUV+ grassConfig.windFrequency* frameData.time);
    vec2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windUV).xy*2 -1  );
	vec3 wind = normalize(vec3(windSample.x, 0, windSample.y));
    vec3 windOffset = 
    mat3x3 windRotation = angleAxis3x3(mix(-PI*0.5*grassConfig.windStrength,PI*0.5*grassConfig.windStrength, length(windSample)), wind);
    */

    /*
    //Double sample
	vec2 tempUV = vec2((position.x / range)*0.5 + 1,(position.z / range)*0.5 + 1)*0.2 ;
    vec2 windUV = fract( tempUV+ grassConfig.windFrequency* frameData.time);
    vec2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windUV).xy*2 -1  );
	//vec3 wind = normalize(vec3(windSample.x, 0, windSample.y));
    vec2 windOffset = 0.01*uv.y *windSample    + windUV;
    windSample = (texture (sampler2D (windTex, colorSampler[2]), windOffset).xy*2 -1  );
	vec3 wind = normalize(vec3(windSample.x, 0, windSample.y));
    mat3x3 windRotation = angleAxis3x3(mix(-PI*0.5*grassConfig.windStrength,PI*0.5*grassConfig.windStrength, length(windSample)), wind);
    */

	vec2 tempUV = vec2((position.x / range)*0.5 + 1,(position.z / range)*0.5 + 1)*0.2 ;
    vec2 windUV = fract( tempUV+ grassConfig.windFrequency* frameData.time);
    //vec2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windUV).xy*2 -1  );
	//vec3 wind = normalize(vec3(windSample.x, 0, windSample.y));
    vec2 windOffset = -0.15*uv.y *grassConfig.windFrequency + windUV;
    vec2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windOffset).xy*2 -1  );
	vec3 wind = normalize(vec3(windSample.x, 0, windSample.y));
    mat3x3 windRotation = angleAxis3x3(mix(-PI*0.5*grassConfig.windStrength,PI*0.5*grassConfig.windStrength, length(windSample)), wind);


    //bending the bleade
    mat3x3 bendRotationMatrix = angleAxis3x3(rand(position.zzx) * grassConfig.grassBend* PI * 0.5, vec3(1, 0, 0));
    mat3x3 transformation =windRotation*facingRotationMatrix * bendRotationMatrix;
    //mat3x3 transformation =facingRotationMatrix * bendRotationMatrix;
    //mat3x3 transformation =facingRotationMatrix;



    //computing the height and width for the blade
    float finalHeight = (rand(position.zyx) * 2.0 - 1.0) * heightRandom+ height;
	float finalWidth = (rand(position.xzy) * 2.0 - 1.0) * widthRandom + width;



	float forward= rand(position.yyz) * grassConfig.bladeForward;
    float segmentForward = pow(uv.y, grassConfig.bladeCurvatureAmount) * forward;
    
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

    outNormal= normalize(transformation*vec3(0,segmentForward,-1));

    vec3 rotatedOffset = transformation*offset.xyz;
	position.xyz += rotatedOffset;
    //position.xyz  *= tileNumber > 300 ? 0 : 1;


	outUV= uv;
    worldPos = position;

	gl_Position = frameData.m_activeCamera.MVP * (vec4(position,1.0));
}

