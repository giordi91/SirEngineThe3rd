#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0,space0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1,space3);


// deferred buffer bindings
Texture2D depthTexture : register(t0,space3);
Texture2D colorSpecIntTexture : register(t1,space3);
Texture2D normalTexture : register(t2,space3);
Texture2D specPowTexture : register(t3,space3);
TextureCube skyboxIrradianceTexture: register(t4,space3);
TextureCube skyboxRadianceTexture: register(t5,space3);
Texture2D brdfTexture: register(t6,space3);
Texture2D directionalShadow: register(t7,space3);

#include "../common/deferredUnpacking.hlsl"
#include "../common/pbr.hlsl"
#include "../common/shadows.hlsl"

float3 CalcWorldPos(float2 csPos, float depth) {
  float4 position;

  position.xy = csPos.xy * g_cameraBuffer.perspectiveValues.xy * depth;
  position.z = depth;
  position.w = 1.0;

  return mul(position, g_cameraBuffer.ViewMatrix).xyz;
}

#define MAX_DEPTH 0.999999f

//==============================================
// PBR
//==============================================


float4 PBRLighting(FullScreenVertexOut input) {
  SURFACE_DATA_PBR gbd = UnpackGBufferPBR(input.uv);
  float4 finalColor = 0.0f;
  float3 ldir = normalize(-g_dirLight.lightDir.xyz);

  if (gbd.depth <= MAX_DEPTH) {
    float3 worldPos = CalcWorldPos(input.clipPos, gbd.linearDepth);

    // camera vector
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float3 reflected = reflect(-toEyeDir, gbd.normal);
    //reflected = gbd.normal;



	float3 albedo =gbd.color; 
	float metallic = gbd.metallic ;
	float roughness = gbd.roughness;
	//metallic = 0.0f;
	//roughness = 0.05f;
    //float3 albedo = float3(0.8f, 0.0f, 0.0f);
	//float metallic = 0.8f;
	//float roughness = 0.1f;


	//float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflected, 0).rgb;


	//float3 albedo =float3(0.8f,0.0f,0.0f); 
	//float metallic = 0.0f;
	//float roughness = 1.0f;

    // fresnel slick, ratio between specular and diffuse, it is tintend on
    // metal, so we lerp toward albedo based on metallic, so only specular will
    // be tinted by the albedo.
    float3 F0 = 0.04f;
    F0 = lerp(F0, albedo, metallic);
    float3 F = fresnelSchlick(max(dot(gbd.normal, toEyeDir), 0.0f), F0, roughness);
    float NDF = DistributionGGX(gbd.normal, halfWay, roughness);
    float G = GeometrySmith(gbd.normal, toEyeDir, ldir, roughness);


    // compute cook torrance
    float3 nominator = NDF * G * F;
    float denominator =
        4.0f * max(dot(gbd.normal, toEyeDir), 0.0f) * max(dot(gbd.normal, ldir), 0.0f) + 0.001f;
    float3 specular = nominator / denominator;

    // compute specular
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;

    // reflectance value
    float3 Lo = 0.0f;
    // single light for now
    float3 radiance = g_dirLight.lightColor.xyz;
    float NdotL = max(dot(gbd.normal, ldir), 0.0f);
        Lo += (kD * (albedo / PI) + specular) * radiance * NdotL;

	float shadowBias = 0.001f;
	float attenuation = samplePCF9taps(worldPos, g_dirLight.lightVP,shadowBias);

	//using irradiance map
	float3 irradiance = skyboxIrradianceTexture.Sample(gsamLinearClamp,gbd.normal).xyz;
	float3 diffuse      = irradiance* albedo;

    float MAX_REFLECTION_LOD = 6.0f;
	float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp,reflected, roughness* MAX_REFLECTION_LOD).rgb;
        float2 envBRDF = brdfTexture.Sample(gsamLinearClamp, float2(max(dot(normalize(gbd.normal), normalize(toEyeDir)), 0.0), roughness)).rg;
    float3 specularDiff = prefilteredColor * (F* envBRDF.x + envBRDF.y);
	specularDiff*= attenuation;


	float3 ambient = (kD * diffuse) + specularDiff;

	//attenuation = 1.0f;

    float3 color = ambient + Lo*attenuation;

	//translucency
	float distortion = 1.0f;
	float translucencyPower = 2.0f;
	float translucencyScale = 0.3f;
	float tAttenuation = 1.0f;
	float tAmbient = 0.00f;
	float3 light = g_dirLight.lightDir.xyz + gbd.normal*distortion;
	float tdot = pow(saturate(dot(toEyeDir, light)), translucencyPower) * translucencyScale;
	float t = gbd.thickness;
	float b = -2.0f;
	//float thickness = 1 / ( 1 + pow(t/(1-t),b));
	float thickness = t;
	float translucency =   (tdot + tAmbient )*thickness ;

	float3 lightScatterColor = g_dirLight.lightColor.xyz;
	//TODO fix hardcoded lightscatter color
	lightScatterColor = float3(0.8f,0.4f,0.4f);
	color += (translucency*gbd.color* lightScatterColor);


	//color = attenuation;
	//if(attenuation < 0.8)
	//	color = float3(1,0,0);

    //float3 color = irradiance*kD;
    //return float4(, 1.0f);
    //return float4(F, 1.0f);
    //return float4(F, 1.0f);
    //return float4(envBRDF,0.0f, 1.0f);
    //return float4(dot(gbd.normal,toEyeDir    ), 0.0f, 0.0f, 1.0f);
    //return float4(F0, 1.0f);
    //return float(max(dot(gbd.normal, ldir), 0.0f),0.0)
    //return float4(.x,0.0f,0.0f, 1.0f);
    
    //return float4(Lo,1.0f);
    //return float4(ambient, 1.0f);
	//return float4(translucency,0.0f,0.0f,1.0f);
    return float4(color, 1.0f);
  }
  return finalColor;
}

float4 PS(FullScreenVertexOut input) : SV_TARGET {
  //return phongLighting(input);
  return PBRLighting(input);
}
