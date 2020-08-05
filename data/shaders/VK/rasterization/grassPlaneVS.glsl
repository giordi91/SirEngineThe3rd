#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "../common/constants.glsl"
#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

layout (set=3,binding=3) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 


layout(location =1) out vec2 outUV;
layout(location =2) out vec3 outNormal;
layout (location = 3) out vec3 worldPos;

const vec4 arrBasePos[6] = {
    vec4(-1.0f, -1.0f, 0.0f, 1.0f), 
	vec4(1.0f, 1.0f, 1.0f, 0.0f),
    vec4(-1.0f, 1.0f, 0.0f, 0.0f), 
	vec4(1.0f, 1.0f, 1.0f, 0.0f),
	vec4(-1.0f, -1.0f, 0.0f, 1.0f),
	vec4(1.0f, -1.0f, 1.0f, 1.0f)
	};

void VS()
{


	vec4 data = arrBasePos[gl_VertexIndex];

	//using grass config to compute points of the plane
    int tilesPerSide = grassConfig.tilesPerSide;
    float halfSize = tilesPerSide*0.5;
    float tw = grassConfig.tileSize;
	vec3 localPos = vec3(data.x,0.0f,data.y);
    vec3 position = grassConfig.gridOrigin + ((halfSize*tw)*localPos);

	outUV = data.zw;
    worldPos = position;
	outNormal = vec3(0,1,0);
	gl_Position = cameraBuffer.MVP * (vec4(position,1.0));
}

