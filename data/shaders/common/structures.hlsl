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
  // TODO pad shiness into ka.w
  float shiness;
  float reflective;
  float roughnessMult;
  float metallicMult;
};

struct DebugLayerConfig {
  int stencilValue;
};

struct GammaToneMappingConfig {
  float exposure;
  float gamma;
  float gammaInverse;
  float padding;
};

struct TextureConfig {
  int width;
  int height;
};
struct ReducedDepth {
  float minDepth;
  float maxDepth;
};

struct SSSSSConfig {
	float sssLevel;
	float correction;
	float maxdd;
	float width;
	float2 direction;
};
#endif
