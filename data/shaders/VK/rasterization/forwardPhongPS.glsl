#version 450

#extension GL_GOOGLE_include_directive: require
#include "../common/structures.glsl"

layout (set=0,binding=0) uniform InputData 
{
	CameraBuffer cameraBuffer;
}; 

layout (set=3,binding = 4) uniform texture2D albedoTex;
layout (set=3,binding = 5) uniform texture2D tangentTex;
layout (set=3,binding = 6) uniform texture2D metallicTex;
layout (set=3,binding = 7) uniform texture2D roughnessTex;

layout (set=3,binding = 8) uniform textureCube skyboxIrradianceTexture;
layout (set=3,binding = 9) uniform textureCube skyboxRadianceTexture;
layout (set=3,binding = 9) uniform textureCube brdfTexture;

//pass data, lights
layout (set=2,binding=0) uniform LightData 
{
	DirectionalLightData lightData;
}; 
layout (set=1,binding = 0) uniform sampler[7] colorSampler;

layout(location=0) out vec4 outputColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 tans;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 worldPos;

const float PI = 3.14159265359;


//F0 is the reflectance at zero angle of incidence, meaning
//how much the surface reflects looking directly at it
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}



void PS()
{
   vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   //light direction, going from the frament out, so in this case 
   //the negative directional light data
   vec3 L = normalize(-lightData.lightDir.xyz);
   //the view direction, from the frament to the camera
   vec3 V = normalize(cameraBuffer.position.xyz - worldPos);
   //half way vector
   vec3 H = normalize(L + V );
   vec3 normal = normalize(inNormal);

   //we don't compute light attenuation for a directional light
   //float dist = length(lightData.lightPosition.xyz - worldPos);
   //float attenuation = 1.0/ dist;
   float attenuation = 1.0;
   vec3 radiance = attenuation *lightData.lightColor.xyz;
   vec3 albedo = texture (sampler2D (albedoTex, colorSampler[2]), uv).xyz;
   float metallic = texture (sampler2D (metallicTex, colorSampler[2]), uv).x;
   float roughness = texture (sampler2D (roughnessTex, colorSampler[2]), uv).x;

   vec3 F0 = vec3(0.04); 
   F0      = mix(F0, albedo, metallic);
   vec3 fresnel  = fresnelSchlick(max(dot(V, normal), 0.0), F0);

   float NDF = DistributionGGX(normal, H, roughness);       
   float G   = GeometrySmith(normal, V, L, roughness); 

   vec3 numerator    = NDF * G * fresnel;
   float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
   vec3 specular     = numerator / max(denominator, 0.001);  

   vec3 kS = fresnel;
   vec3 kD = vec3(1.0) - fresnel;
  
   kD *= 1.0 - metallic;	

   float NdotL = max(dot(normal, L), 0.0);        
   vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
   
   vec3 ambient = vec3(0.03) * albedo;
   vec3 color   = ambient + Lo;  

   outputColor = vec4(color,1.0f);
}
