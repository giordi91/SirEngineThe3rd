#ifndef STRUCTURES_HLSL
#define STRUCTURES_HLSL

struct Plane
{
	float4 position;
    float4 normal;
};


struct CameraBuffer {
  Plane frustum[6];
  float4x4 MVP;
  float4x4 ViewMatrix;
  float4x4 VPinverse;
  float4 perspectiveValues;
  float4 position;
  float4 cameraViewDir;
  float3 padding;
  float vFov;
};

//this struct should hold all the data that changes once per frame or less
struct FrameData
{
    CameraBuffer m_mainCamera;
    CameraBuffer m_activeCamera;
    float time;
    float screenWidth;
    float screenHeight;
    float padding;
};

struct DirectionalLightData {
  float4x4 projectionMatrix;
  float4x4 lightVP;
  float4x4 worldToLocal;
  float4x4 localToWorld;
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

struct DebugPointsFixedColor {
  float4 color;
  float pointSize;
  float3 padding;
};

struct BoundingBox {
  float3 min;
  float3 max;
  float2 padding;
};

struct GrassConfig {
  float3 gridOrigin;

  //tile config
  float tileSize;

  int tilesPerSide;
  int sourceDataTileCount;
  int seed;
  int pointsPerTile;

  int pointsPerSourceTile;

  //blade config
  float bladeForward;
  float bladeCurvatureAmount;
  float grassBend;
  float widthRandom;
  float heightRandom;
  float width;
  float height;

  //wind config
  float2 windFrequency;
  float windStrength;
  float padding;
  //shading
  float3 baseColor;
  float roughness;
  float3 tipColor;
  float metalness;

  //lods
  float4 lodThresholds;
};
#endif
