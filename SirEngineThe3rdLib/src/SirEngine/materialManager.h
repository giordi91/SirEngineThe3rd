#pragma once

#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/hashMap.h"

namespace SirEngine {

struct ShaderBind {
  RSHandle rs;
  PSOHandle pso;
};

struct MaterialDataHandles {
  TextureHandle albedo;
  TextureHandle normal;
  TextureHandle metallic;
  TextureHandle roughness;
  TextureHandle thickness;
  TextureHandle separateAlpha;
  TextureHandle ao;
  TextureHandle height;
  ConstantBufferHandle cbHandle;
  SkinHandle skinHandle;
};

enum class STENCIL_REF { CLEAR = 0, SSSSS = 1 };
#define INVALID_QUEUE_TYPE_FLAGS 0xFFFFFFFF

// TODO revise this struct
struct Material final {
  float kDR;
  float kDG;
  float kDB;
  float dPadding;
  float kAR;
  float kAG;
  float kAB;
  float aPadding;
  float kSR;
  float kSG;
  float kSB;
  float sPadding;
  float shiness;
  float reflective;
  float roughnessMult;
  float metallicMult;
};

enum class SHADER_TYPE_FLAGS {
  UNKNOWN = 0,
  PBR,
  SKIN,
  FORWARD_PBR,
  FORWARD_PHONG,
  FORWARD_PHONG_ALPHA_CUTOUT_SKIN,
  FORWARD_PHONG_ALPHA_CUTOUT,
  HAIR,
  DEBUG_POINTS_SINGLE_COLOR,
  DEBUG_POINTS_COLORS,
  DEBUG_LINES_SINGLE_COLOR,
  DEBUG_LINES_COLORS,
  DEBUG_TRIANGLE_SINGLE_COLOR,
  DEBUG_TRIANGLE_COLORS,
  SKINCLUSTER,
  SKINSKINCLUSTER,
  HAIRSKIN,
  FORWARD_PARALLAX,
  SHADOW_SKIN_CLUSTER,
  HDR_TO_SDR,
  GRASS_FORWARD
};

class MaterialManager {
public:
  static constexpr uint32_t QUEUE_COUNT = 5;

protected:
  struct PrelinaryMaterialParse {
    Material mat;
    MaterialDataHandles handles;
    uint32_t shaderQueueTypeFlags[QUEUE_COUNT] = {
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS};
    bool isStatic = false;
  };

public:
  enum ALLOCATE_MATERIAL_FLAG_BITS { NONE = 0, BUFFERED = 1 };
  typedef uint32_t ALLOCATE_MATERIAL_FLAGS;

public:
  MaterialManager(const uint32_t reserveSize)
      : m_shaderTypeToShaderBind(reserveSize){};
  virtual ~MaterialManager() = default;

  void loadTypesInFolder(const char *folder);
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  virtual void inititialize() = 0;
  virtual void cleanup() = 0;

  virtual MaterialHandle
  allocateMaterial(const char *name, ALLOCATE_MATERIAL_FLAGS flags,
                   const char *materialsPerQueue[QUEUE_COUNT]) = 0;
  virtual MaterialHandle loadMaterial(const char *path,
                                      const MeshHandle meshHandle,
                                      const SkinHandle skinHandle) = 0;
  virtual void bindMaterial(MaterialHandle handle,
                            SHADER_QUEUE_FLAGS queue) = 0;
  virtual void bindTexture(MaterialHandle matHandle, TextureHandle texHandle,
                           uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) = 0;
  virtual void bindBuffer(MaterialHandle matHandle, BufferHandle texHandle,
                           uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) = 0;
  virtual void free(MaterialHandle handle) = 0;

  inline SHADER_TYPE_FLAGS getTypeFlags(const uint32_t flags) {
    // here we are creating a mask for the fist 16 bits, then we flip it
    // such that we are going to mask the upper 16 bits
    constexpr uint32_t mask = static_cast<uint32_t>(~((1 << 16) - 1));
    const uint32_t typeFlags = (flags & mask) >> 16;
    return static_cast<SHADER_TYPE_FLAGS>(typeFlags);
  }

  inline bool isShaderOfType(const uint32_t flags,
                             const SHADER_TYPE_FLAGS type) {
    const SHADER_TYPE_FLAGS typeFlags = getTypeFlags(flags);
    return typeFlags == type;
  }

  inline uint32_t getQueueFlags(const uint32_t flags) {
    constexpr uint32_t mask = (1 << 16) - 1;
    const uint32_t queueFlags = flags & mask;
    return queueFlags;
  }

  inline bool isQueueType(const uint32_t flags,
                          const SHADER_QUEUE_FLAGS queue) {
    const uint32_t queueFlags = getQueueFlags(flags);
    return (queueFlags & static_cast<uint32_t>(queue)) > 0;
  }

  inline uint32_t getQueueTypeFlags(SHADER_QUEUE_FLAGS queue,
                                    uint32_t shaderType) {
    uint32_t flags = 0;
    uint32_t currentFlag = static_cast<uint32_t>(queue);
    flags |= currentFlag;

    int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

    flags = shaderType << 16 | flags;
    return flags;
  }

  static uint16_t parseTypeFlags(const char *stringType);

  const char *getStringFromShaderTypeFlag(SHADER_TYPE_FLAGS type);

protected:
  static PrelinaryMaterialParse parseMaterial(const char *path,
                                       const MeshHandle meshHandle,
                                       const SkinHandle skinHandle);
  void loadTypeFile(const char *path);

protected:
  HashMap<uint16_t, ShaderBind, hashUint16> m_shaderTypeToShaderBind;
};

} // namespace SirEngine
