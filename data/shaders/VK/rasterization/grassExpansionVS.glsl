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

layout (set=3,binding=0) buffer vertices
{
	vec4 p[];
};

layout (set=3,binding = 1) uniform texture2D windTex;
layout (set=1,binding = 0) uniform sampler[7] colorSampler;


    //vec4(1.0f, 0.0f, 0.0f, width), 
    //vec4(0.0f, 1.0f, 0.0f, height), 
    //vec4(-1.0f, 0.0f, 0.0f, width), 
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


//config to become a buffer
const float grassBend = 0.2f;
const float width = 0.1;
const float height = 1.5;
const float widthRandom = 0.02;
const float heightRandom = 0.1;
const float windStrength = 0.3f;
const vec2 windFrequency = vec2(0.1,0.1);
const float bladeForward = 1.0;
const float bladeCurvatureAmount = 2.0;


layout(location =1) out vec2 outUV;
void VS()
{
    uint vid = gl_VertexIndex/15;
	uint localId = gl_VertexIndex%15;
	vec4 position = p[vid];
	vec4 offset = vec4(offsets[localId],0.0f);



    //spinning the blade by random amount
    mat3x3 facingRotationMatrix = angleAxis3x3(rand(position.xyz) * TWO_PI, vec3(0, 1, 0));

    //sampling the wind texture
    //hardcoded range [-15,15]
    float range = 15;

	vec2 tempUV = vec2((position.x / range)*0.5 + 1,(position.z / range)*0.5 + 1) * 0.1;
    vec2 windUV = fract( tempUV+ windFrequency* cameraBuffer.time);
    vec2 windSample = (texture (sampler2D (windTex, colorSampler[2]), windUV).xy * 2 -1)*windStrength;
	vec3 wind = normalize(vec3(windSample.y, 0, windSample.x));
    mat3x3 windRotation = angleAxis3x3(PI*0.5 * length(windSample), wind);

    //bending the bleade
    mat3x3 bendRotationMatrix = angleAxis3x3(rand(position.zzx) * grassBend* PI * 0.5, vec3(1, 0, 0));
    mat3x3 transformation =windRotation*facingRotationMatrix * bendRotationMatrix;
    //mat3x3 transformation =facingRotationMatrix * bendRotationMatrix;
    //mat3x3 transformation =facingRotationMatrix;



    //computing the height and width for the blade
    float finalHeight = (rand(position.zyx) * 2.0 - 1.0) * heightRandom+ height;
	float finalWidth = (rand(position.xzy) * 2.0 - 1.0) * widthRandom + width;


    vec2 uv = bladeUVs[localId];

	float forward= rand(position.yyz) * bladeForward;
    float segmentForward = pow(uv.y, bladeCurvatureAmount) * forward;
    
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

    vec3 rotatedOffset = transformation*offset.xyz;
    //vec3 rotatedOffset = offset.xyz;
	position.xyz += rotatedOffset;


	outUV= uv;

	gl_Position = cameraBuffer.MVP * (position);
}

