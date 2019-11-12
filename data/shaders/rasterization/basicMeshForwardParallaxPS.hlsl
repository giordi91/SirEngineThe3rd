#include "../common/pbr.hlsl"
#include "../common/normals.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1);
ConstantBuffer<PhongMaterial> g_material : register(b2);
Texture2D albedoTex : register(t0);
Texture2D tangentTex : register(t1);
Texture2D metallicTex : register(t2);
Texture2D roughnessTex : register(t3);
TextureCube skyboxIrradianceTexture : register(t4);
TextureCube skyboxRadianceTexture : register(t5);
Texture2D brdfTexture : register(t6);
Texture2D heightTexture : register(t7);
Texture2D directionalShadow: register(t8);


SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);


float2 parallax(float2 uv, float3 viewDir, float height, float heightScale)
{
    float2 p = (viewDir.xy / viewDir.z) * (height * heightScale);
    return uv - p;
}
float2 steepParallax(float2 uv, float3 viewDir, float height, float heightScale)
{

     // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    float2 P = viewDir.xy / viewDir.z * heightScale;
    float2 deltaTexCoords = P / numLayers;
  
    // get initial values
    float2 currentTexCoords = uv;
    float currentDepthMapValue = 1.0 - heightTexture.Sample(gsamLinearWrap, currentTexCoords).x;
      
    bool run = true;
    int layer = 0;
    while (run)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get height value at current texture coordinates
        currentDepthMapValue = 1.0 - heightTexture.Sample(gsamLinearWrap, currentTexCoords).x;
        // get depth of next layer
        currentLayerDepth += layerDepth;
        ++layer;
        run = (currentLayerDepth < currentDepthMapValue) & (layer < numLayers);
    }
    
    return currentTexCoords;
}
float2 parallaxOcclusionMapping(float2 uv, float3 viewDir, float height, float heightScale)
{
     // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0.0, 0.0, 1.0), viewDir)));
    //float numLayers = 100;
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    float2 P = viewDir.xy / viewDir.z * heightScale;
    float2 deltaTexCoords = P / numLayers;
  
    // get initial values
    float2 currentTexCoords = uv;
    //float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
    float currentDepthMapValue = 1.0 - heightTexture.Sample(gsamLinearWrap, currentTexCoords).x;
      
    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        //currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        currentDepthMapValue = 1.0 - heightTexture.Sample(gsamLinearWrap, currentTexCoords).x;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }
    
    // get texture coordinates before collision (reverse operations)
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue;
    float beforeDepth = (1.0 - heightTexture.Sample(gsamLinearWrap, prevTexCoords).x) - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;

    
}


float4 PS(FullMeshParallaxVertexOut input) : SV_Target
{
    //TODO need to clean up those parameters!
    float uvScale = 4.0f;
    float heightScale = 0.035;

    float2 uv = float2(input.uv.x, input.uv.y) * uvScale;
    float height = 1.0f - heightTexture.Sample(gsamLinearWrap, uv).x;

    float3 toEyeTangent = normalize(input.tangentViewPos.xyz - input.tangentFragPos.xyz).xyz;
    float2 parallaxUV = parallaxOcclusionMapping(uv, toEyeTangent, height, heightScale);
    float3 albedo = albedoTex.Sample(gsamLinearWrap, parallaxUV).xyz * g_material.kd.xyz;


    float3 view = toEyeTangent.xyz;
    float lengthV =
    length((view.xy / view.z * (height * heightScale)));
    lengthV = lengthV > 1.0f ? 1 : 0;
    float2 p = (view.xy / view.z) * (height * heightScale);

    float3 texNormal =
      normalize(tangentTex.Sample(gsamLinearWrap, parallaxUV) * 2.0f - 1.0f).xyz;
    // compute NTB
    float3 N = normalize(input.Normal.xyz);
    float3 T = normalize(input.tangent.xyz);
    float3 normal = computeNormalFromNormalMap(N, T, texNormal);
    //TODO fix this
    N = normal;

    // sampling PBR textures
    float metallic =
      metallicTex.Sample(gsamLinearWrap, parallaxUV).x * g_material.metallicMult;
    float roughness =
      roughnessTex.Sample(gsamLinearWrap, parallaxUV).x * g_material.roughnessMult;

    // view vectors
    float3 worldPos = input.worldPos.xyz;

	//lets check shadows
	float4 shadowMapPos = mul(float4(worldPos,1.0),g_dirLight.lightVP);
	shadowMapPos  /= shadowMapPos.w;
	//convert to uv values
	float2 shadowUV= 0.5f * shadowMapPos.xy + 0.5f;
	//float2 shadowUV= uv/4;
	shadowUV.y = 1.0f - shadowUV.y;

	//sample the shadow
    float shadowDepth =
      directionalShadow.Sample(gsamLinearClamp, shadowUV).x;

	float attenuation = shadowDepth > shadowMapPos.z ? 0.0f : 1.0f;
	//float shwx = shadowUV.x;
	//float shwy = shadowUV.y;
	//bool outOfRangeShadow = (shwx > 1.0f) | (shwx < 0.0f) | (shwy < 0.0f) | (shwy > 1.0f);
	//attenuation = outOfRangeShadow ? 1.0f : attenuation;

    // camera vector
    float3 ldir = normalize(-g_dirLight.lightDir.xyz);
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float3 reflected = reflect(-toEyeDir, N);

    // fresnel slick, ratio between specular and diffuse, it is tintend on
    // metal, so we lerp toward albedo based on metallic, so only specular will
    // be tinted by the albedo.
    float3 F0 = 0.04f;
    F0 = lerp(F0, albedo, metallic);
    float3 F = fresnelSchlick(max(dot(N, toEyeDir), 0.0f), F0, roughness);
    float NDF = DistributionGGX(N, halfWay, roughness);
    float G = GeometrySmith(N, toEyeDir, ldir, roughness);

    // compute cook torrance
    float3 nominator = NDF * G * F;
    float denominator =
  	4.0f * max(dot(N, toEyeDir), 0.0f) * max(dot(N, ldir), 0.0f) + 0.001f;
    float3 specular = nominator / denominator;
  
    // compute specular
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
  
    // reflectance value
    float3 Lo = 0.0f;
    // single light for now
    float3 radiance = g_dirLight.lightColor.xyz;
    float NdotL = max(dot(N, ldir), 0.0f);
    Lo += (kD * (albedo / PI) + specular) * radiance * NdotL;
  
    //using irradiance map
    float3 irradiance = skyboxIrradianceTexture.Sample(gsamLinearClamp, N).xyz;
    float3 diffuse = irradiance * albedo;
  
    float MAX_REFLECTION_LOD = 6.0f;
    float3 prefilteredColor = skyboxRadianceTexture.SampleLevel(gsamLinearClamp, reflected, roughness * MAX_REFLECTION_LOD).rgb;
    float2 envBRDF = brdfTexture.Sample(gsamLinearClamp, float2(max(dot(normalize(N), normalize(toEyeDir)), 0.0), roughness)).rg;
    float3 specularDiff = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    float3 ambient = (kD * diffuse) + (specularDiff);
    float3 color = ambient + attenuation* Lo;
	//color = float3(shadowUV,0);
	//color = shadowDepth;

    return float4(color, 1.0f);
}
