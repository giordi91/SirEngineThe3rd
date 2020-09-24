#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/structures.glsl"
#include "../common/pbr.glsl"

layout(location=0) out vec4 outputColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 worldPos;
layout (location = 4) flat in int lod;

layout (set=0,binding=0) uniform InputData 
{
	FrameData frameData;
}; 

//pass data, lights
layout (set=2,binding=0) uniform LightData 
{
	DirectionalLightData lightData;
}; 

layout (set=3,binding=3) uniform ConfigData 
{
	GrassConfig grassConfig;
}; 

layout (set=3,binding = 4) uniform texture2D albedoTex;

//pass data
layout (set=1,binding = 0) uniform sampler[7] colorSampler;
layout (set=2,binding = 1) uniform textureCube skyboxIrradianceTexture;
layout (set=2,binding = 2) uniform textureCube skyboxRadianceTexture;
layout (set=2,binding = 3) uniform texture2D brdfTexture;



void PS()
{
   //light direction, going from the frament out, so in this case 
   //the negative directional light data
   vec3 L = normalize(-lightData.lightDir.xyz);
   //the view direction, from the frament to the camera
   vec3 V = normalize(frameData.m_activeCamera.position.xyz - worldPos);
   //half way vector
   vec3 H = normalize(L + V );
   vec3 normal = normalize(dot(inNormal,L) < 0.0 ? -inNormal : inNormal);

   //we don't compute light attenuation for a directional light
   //float dist = length(lightData.lightPosition.xyz - worldPos);
   //float attenuation = 1.0/ dist;
   float attenuation = 1.0;
   vec3 radiance = attenuation *lightData.lightColor.xyz;
   vec3 albedo = texture (sampler2D (albedoTex, colorSampler[2]), inUV).xyz;
   float metallic = grassConfig.metalness;
   float roughness = grassConfig.roughness;

   //the initial F0 is the basic reflectivity when looking straight at the material
   //zero incidence of the view vector. The base  reflectivity is compute from the IOR (index of refraction)
   //now the problem is that this computation only really works for not metallic materials (dialectrics)
   //so to get around that we use the incidence zero reflectivity and approximate such that will work for both
   //dialectrics and conductors (metallic)
   vec3 F0 = vec3(0.04); 
   //to note, metallics do not have a common F0, but is tinted, so we use the metallic
   //property to tin the F0 based on the albedo color.
   F0      = mix(F0, albedo, metallic);
   //fresnel gives us the reflectance ast grazing angle, that is why we  pass in the 
   //cosine factor dot(V,N) between the view and the normal. 
   //more correctly it tells how much of the ligth gets reflected and how much gets refracted
   //give the looking direction.
   vec3 fresnel  = fresnelSchlick(max(dot(V, H), 0.0), F0, roughness);
   //outputColor = vec4(fresnel,1.0f);
   //return;

   
   //here we compute the normal distribution, meaning how many (statistically speaking) 
   //microfactes are aligned to the half vector based on the roughness. Smoother material
   //will give the bright specular, rough material will have more diffuse appearance
   float NDF = DistributionGGX(normal, H, roughness);       
   //here we approximate how many of the microfacets self shadow themselves
   float G   = GeometrySmith(normal, V, L, roughness); 

   //computing the cook torrance part of the brdf now that we have all the components
   vec3 numerator    = NDF * G * fresnel;
   float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
   vec3 specular     = numerator / max(denominator, 0.001);  

   //as we mentioned above the fresnel gives us the amount of light that get reflected,
   //as such assuming total light is 1 we can compute amount of light that gets refracted
   vec3 kS = fresnelSchlick(max(dot(V, normal), 0.0), F0, roughness);
   vec3 kD = vec3(1.0) - fresnel;
  
   //since metallic surfaces do not deffuse light but fully absorbe it, we are going 
   //to nullify the kd based on how metallic the surfaces is
   kD *= 1.0 - metallic;	

   //scaling the value with the cosine rule
   float NdotL = max(dot(normal, L), 0.0);        

   //computing the final brdf value
   vec3 lambert = albedo / PI;
   vec3 Lo = (kD *lambert  + specular) * radiance * NdotL;

   //Lo += dot(normal,L) < 0.0? albedo*0.1 : vec3(0,0,0);

   //adding the ambient color, we sample the irradiance map using the normal texture, we weight the amount based on
   //the kd
   vec3 irradiance = texture(samplerCube (skyboxIrradianceTexture, colorSampler[2]), normal).xyz;
   vec3 diffuse = irradiance*albedo;


   //doing specular reflection
   vec3 reflection = reflect (-V,normal);
   const float MAX_REFLECTION_LOD =8.0;
   vec3 prefilteredColor = textureLod(samplerCube (skyboxRadianceTexture, colorSampler[2]),reflection,
	roughness*MAX_REFLECTION_LOD).xyz;

	vec2 brdf = texture (sampler2D (brdfTexture, colorSampler[2]), vec2(max(dot(normal, V), 0.0), roughness)).xy;
	vec3 prefilteredAmount = (fresnel * brdf.x + brdf.y);
    vec3 specularBrdf = prefilteredColor * (kS* brdf.x + brdf.y);

	//vec3 specularBrdf = pow(clamp(dot(normal,H),0,1),150) *vec3(0.8,0.8,0.8);
	//specularBrdf*= pow(inUV.y,4);
    vec3 ambient = (kD * diffuse + specularBrdf);



   vec3 finalC = ambient + Lo;  
   //finalC = dot(normal,L) < 0.0? albedo*0.1 : finalC;

   //hear we are using the v value to diminis the light received by the blades 
   //due to be missing shadows
   float attenuationFactor =clamp(pow(inUV.y + 0.2,3),0,1);
   
   outputColor = vec4(finalC*attenuationFactor,1.0f);
   if(false){
	   if(lod ==0) { outputColor = vec4(1,0,0,1);}
	   if(lod ==1) { outputColor = vec4(0,1,0,1);}
	   if(lod ==2) { outputColor = vec4(0,0,1,1);}
	   if(lod ==3) { outputColor = vec4(1,0,1,1);}
   }

}
