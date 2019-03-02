#ifndef STRUCTURES_HLSL 
#define STRUCTURES_HLSL

struct CameraBuffer {
  float4x4 MVP;
  float4x4 ViewMatrix;
  float4x4 VPinverse;
  float4 perspectiveValues;
  float4 position;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;
};

struct DirectionalLightData {
  float4x4 lightVP;
  float4 lightPosition;
  float4 lightDir;
  float4 lightColor;
};

// material buffer
struct PhongMaterial {
  float4 kd;
  float4 ka;
  float4 ks;
  // todo padd shiness into ka.w
  float shiness;
  float reflective;
  float padding2;
  float padding3;
};
#endif
