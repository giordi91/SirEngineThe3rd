#pragma once

#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/cpu/hashMap.h"

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
  GRASS_FORWARD,
  SKYBOX,
  GAMMA_AND_TONE_MAPPING
};

class MaterialManager {
 public:
  static constexpr uint32_t QUEUE_COUNT = 5;

 protected:
  struct PreliminaryMaterialParse {
    Material mat;
    MaterialDataHandles handles;
    uint32_t shaderQueueTypeFlags[QUEUE_COUNT] = {
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS};
    // defines whether or not we expect the material to change, if not we can
    // save some resource allocations
    bool isStatic = false;
  };

 public:
  enum ALLOCATE_MATERIAL_FLAG_BITS { NONE = 0, BUFFERED = 1 };
  typedef uint32_t ALLOCATE_MATERIAL_FLAGS;

 public:
  explicit MaterialManager(const uint32_t reserveSize)
      : m_shaderTypeToShaderBind(reserveSize){};
  virtual ~MaterialManager() = default;

  void loadTypesInFolder(const char *folder);
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  virtual void inititialize() = 0;
  virtual void cleanup() = 0;

  virtual MaterialHandle allocateMaterial(
      const char *name, ALLOCATE_MATERIAL_FLAGS flags,
      const char *materialsPerQueue[QUEUE_COUNT]) = 0;
  virtual MaterialHandle loadMaterial(const char *path,
                                      const MeshHandle meshHandle,
                                      const SkinHandle skinHandle) = 0;
  virtual void bindMaterial(MaterialHandle handle,
                            SHADER_QUEUE_FLAGS queue) = 0;
  virtual void bindTexture(const MaterialHandle matHandle,
                           const TextureHandle texHandle,
                           const uint32_t descriptorIndex,
                           const uint32_t bindingIndex,
                           SHADER_QUEUE_FLAGS queue, const bool isCubeMap) = 0;
  virtual void bindBuffer(MaterialHandle matHandle, BufferHandle texHandle,
                          uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) = 0;
  virtual void bindConstantBuffer(MaterialHandle handle,
                          ConstantBufferHandle bufferHandle,
                          const uint32_t descriptorIndex,
                          const uint32_t bindingIndex,
                          SHADER_QUEUE_FLAGS queue) = 0;

  // TODO clean this. The issue is, we need two indices to bind the mesh
  // one is the binding index, meaning which slot in the shader is referring to.
  // The second one is which descriptor are we going to update?
  // DX12 has a match one to one, we have a series of descriptors exactly as in
  // the PSO for example if you have in the PSO fist a texture then a buffer
  // descriptor for the position of the mesh, you need to have a descriptorIndex
  // of 1 because it appears after the texture vulkan on the other hand is not,
  // you have descriptors that describe a resource and they can be in different
  // order, so if you want to update the mesh you can simply grab an array of
  // descriptor info fill it correctly and do an update. This mean that you have
  // a sort of mismatched indices. Ideally this will be solved by having
  // introspection in the PSO, where you say bind "positions" and will tell you
  // all you need to know, both descriptor and slot index. For now we expose
  // both indices to make it work on both dx12 and VK
  virtual void bindMesh(const MaterialHandle handle,
                        const MeshHandle meshHandle,
                        const uint32_t descriptorIndex,
                        const uint32_t bindingIndex,
                        const uint32_t meshBindFlags,
                        SHADER_QUEUE_FLAGS queue) = 0;

  virtual void free(MaterialHandle handle) = 0;

  static inline SHADER_TYPE_FLAGS getTypeFlags(const uint32_t flags) {
    // here we are creating a mask for the fist 16 bits, then we flip it
    // such that we are going to mask the upper 16 bits
    constexpr uint32_t mask = static_cast<uint32_t>(~((1 << 16) - 1));
    const uint32_t typeFlags = (flags & mask) >> 16;
    return static_cast<SHADER_TYPE_FLAGS>(typeFlags);
  }

  static inline bool isShaderOfType(const uint32_t flags,
                                    const SHADER_TYPE_FLAGS type) {
    const SHADER_TYPE_FLAGS typeFlags = getTypeFlags(flags);
    return typeFlags == type;
  }

  static inline uint32_t getQueueFlags(const uint32_t flags) {
    constexpr uint32_t mask = (1 << 16) - 1;
    const uint32_t queueFlags = flags & mask;
    return queueFlags;
  }

  static inline bool isQueueType(const uint32_t flags,
                                 const SHADER_QUEUE_FLAGS queue) {
    const uint32_t queueFlags = getQueueFlags(flags);
    return (queueFlags & static_cast<uint32_t>(queue)) > 0;
  }

  // combines the queue and shader type flags together
  static inline uint32_t getQueueTypeFlags(SHADER_QUEUE_FLAGS queue,
                                           const uint32_t shaderType) {
    uint32_t flags = 0;
    auto currentFlag = static_cast<uint32_t>(queue);
    flags |= currentFlag;

    flags = shaderType << 16 | flags;
    return flags;
  }

  static uint16_t parseTypeFlags(const char *stringType);

  static const char *getStringFromShaderTypeFlag(SHADER_TYPE_FLAGS type);

 protected:
  static PreliminaryMaterialParse parseMaterial(const char *path,
                                                const MeshHandle meshHandle,
                                                const SkinHandle skinHandle);
  void loadTypeFile(const char *path);

 protected:
  HashMap<uint16_t, ShaderBind, hashUint16> m_shaderTypeToShaderBind;
};

}  // namespace SirEngine
