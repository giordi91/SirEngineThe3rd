#ifndef PARALLAX_HLSL 
#define PARALLAX_HLSL

#include "../common/textureSamplers.hlsl"

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
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0 - heightTexture.Sample(gsamLinearWrap, prevTexCoords).x) - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

#endif
