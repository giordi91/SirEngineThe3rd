#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"
#include "../common/pbr.hlsl"

ConstantBuffer<FrameData> g_frameData : register(b0, space0);
ConstantBuffer<DirectionalLightData> g_dirLightData : register(b0, space2);

ConstantBuffer<GrassConfig> grassConfig : register(b3, space3);
Texture2D albedoTex : register(t4, space3);
TextureCube skyboxIrradianceTexture : register(t1, space2);
TextureCube skyboxRadianceTexture : register(t2, space2);
Texture2D brdfTexture : register(t3, space2);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

float4 PS(PosNormalUVVertexOut pin) : SV_Target {

   //light direction, going from the frament out, so in this case 
   //the negative directional light data
   float3 L = normalize(-g_dirLightData.lightDir.xyz);
   //the view direction, from the frament to the camera
   float3 V = normalize(g_frameData.m_activeCamera.position.xyz - pin.worldPos.xyz);
   //half way vector
   float3 H = normalize(L + V );
   float3 normal = normalize(dot(pin.Normal.xyz,L) < 0.0 ? -pin.Normal.xyz: pin.Normal.xyz);


   float attenuation = 1.0;
   float3 radiance = attenuation *g_dirLightData.lightColor.xyz;
   float3 albedo = albedoTex.Sample(gsamLinearClamp, pin.uv).xyz;
   float metallic = grassConfig.metalness;
   float roughness = grassConfig.roughness;

   //the initial F0 is the basic reflectivity when looking straight at the material
   //zero incidence of the view vector. The base  reflectivity is compute from the IOR (index of refraction)
   //now the problem is that this computation only really works for not metallic materials (dialectrics)
   //so to get around that we use the incidence zero reflectivity and approximate such that will work for both
   //dialectrics and conductors (metallic)
   float3 F0 = float3(0.04,0.04,0.04); 
   //to note, metallic materials do not have a common F0, but is tinted, so we use the metallic
   //property to tin the F0 based on the albedo color.
   F0      = lerp(F0, albedo, metallic);
   //fresnel gives us the reflectance ast grazing angle, that is why we  pass in the 
   //cosine factor dot(V,N) between the view and the normal. 
   //more correctly it tells how much of the ligth gets reflected and how much gets refracted
   //give the looking direction.
   float3 fresnel  = fresnelSchlick(max(dot(V, H), 0.0), F0, roughness);
   //here we compute the normal distribution, meaning how many (statistically speaking) 
   //microfactes are aligned to the half vector based on the roughness. Smoother material
   //will give the bright specular, rough material will have more diffuse appearance
   float NDF = DistributionGGX(normal, H, roughness);       
   //here we approximate how many of the microfacets self shadow themselves
   float G   = GeometrySmith(normal, V, L, roughness); 

   //computing the cook torrance part of the brdf now that we have all the components
   float3 numerator    = NDF * G * fresnel;
   float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
   float3 specular     = numerator / max(denominator, 0.001);  

   //as we mentioned above the fresnel gives us the amount of light that get reflected,
   //as such assuming total light is 1 we can compute amount of light that gets refracted
   float3 kS = fresnelSchlick(max(dot(V, normal), 0.0), F0, roughness);
   float3 kD = 1.0 - fresnel;

   //since metallic surfaces do not deffuse light but fully absorbe it, we are going 
   //to nullify the kd based on how metallic the surfaces is
   kD *= 1.0 - metallic;	

   //scaling the value with the cosine rule
   float NdotL = max(dot(normal, L), 0.0);        

   //computing the final brdf value
   float3 lambert = albedo / PI;
   float3 Lo = (kD *lambert  + specular) * radiance * NdotL;

  //using irradiance map
  float3 irradiance = skyboxIrradianceTexture.Sample(gsamLinearClamp,normal).xyz;
  float3 diffuse      = irradiance* albedo;


  float MAX_REFLECTION_LOD = 8.0f;
   float3 reflection = reflect (-V,normal);
  float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflection, roughness* MAX_REFLECTION_LOD).rgb;
  float2 envBRDF = brdfTexture.Sample(gsamLinearClamp, float2(max(dot(normalize(normal), normalize(V)), 0.0), roughness)).rg;
  float3 specularBrdf= prefilteredColor * (fresnel* envBRDF.x + envBRDF.y);

	
  float3 ambient = (kD * diffuse + specularBrdf);
  float3 finalC = ambient + Lo;  
  //hear we are using the v value to diminis the light received by the blades 
  //due to be missing shadows
  float3 attenuationFactor = saturate(pow(pin.uv.y +0.2,3));
  return float4(finalC*attenuationFactor,1.0f);
}
