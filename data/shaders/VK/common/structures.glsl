#ifndef STRUCTURES_HLSL
#define STRUCTURES_HLSL

struct CameraBuffer {
  mat4 MVP;
  mat4 ViewMatrix;
  mat4 VPinverse;
  vec4 perspectiveValues;
  vec4 position;
  vec4 cameraViewDir;
  float vFov;
  float screenWidth;
  float screenHeight;
  float time;
};

//this struct should hold all the data that changes once per frame or less
struct FrameData
{
    CameraBuffer m_camera;
};

struct DirectionalLightData {
  mat4 projectionMatrix;
  mat4 lightVP;
  mat4 worldToLocal;
  mat4 localToWorld;
  vec4 lightPosition;
  vec4 lightDir;
  vec4 lightColor;
};

// material buffer
struct PhongMaterial {
  vec4 kd;
  vec4 ka;
  vec4 ks;
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
  vec2 direction;
};

struct DebugPointsFixedColor {
  vec4 color;
  float pointSize;
  vec3 padding;
};

struct BoundingBox {
  vec3 min;
  vec3 max;
  vec2 padding;
};
#endif
