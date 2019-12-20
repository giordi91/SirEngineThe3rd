#pragma once

#include "SirEngine/handle.h"
#include "memory/SparseMemoryPool.h"
#include <cassert>
#include <unordered_map>
#include <vector>

#if GRAPHICS_API == DX12
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#endif

namespace SirEngine {

struct ShaderBind {
  RSHandle rs;
  PSOHandle pso;
};

enum class STENCIL_REF { CLEAR = 0, SSSSS = 1 };
#define INVALID_QUEUE_TYPE_FLAGS 0xFFFFFFFF

struct MaterialRuntime final {
  D3D12_GPU_VIRTUAL_ADDRESS cbVirtualAddress;
  D3D12_GPU_DESCRIPTOR_HANDLE albedo;
  D3D12_GPU_DESCRIPTOR_HANDLE normal;
  D3D12_GPU_DESCRIPTOR_HANDLE metallic;
  D3D12_GPU_DESCRIPTOR_HANDLE roughness;
  D3D12_GPU_DESCRIPTOR_HANDLE thickness;
  D3D12_GPU_DESCRIPTOR_HANDLE separateAlpha;
  D3D12_GPU_DESCRIPTOR_HANDLE heightMap;
  D3D12_GPU_DESCRIPTOR_HANDLE ao;
  uint32_t shaderQueueTypeFlags[4] = {
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS};
  SkinHandle skinHandle;
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
  dx12::DescriptorPair albedoSrv;
  dx12::DescriptorPair normalSrv;
  dx12::DescriptorPair metallicSrv;
  dx12::DescriptorPair roughnessSrv;
  dx12::DescriptorPair thicknessSrv;
  dx12::DescriptorPair separateAlphaSrv;
  dx12::DescriptorPair aoSrv;
  dx12::DescriptorPair heightSrv;
  ConstantBufferHandle cbHandle;
  SkinHandle skinHandle;
};
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

enum class SHADER_QUEUE_FLAGS {
  FORWARD = 1 << 0,
  DEFERRED = 1 << 1,
  SHADOW = 1 << 2,
  DEBUG = 1 << 3,
};

enum class SHADER_TYPE_FLAGS {
  UNKNOWN = 0,
  PBR = 1,
  SKIN = 2,
  FORWARD_PBR = 3,
  FORWARD_PHONG_ALPHA_CUTOUT = 4,
  HAIR = 5,
  DEBUG_POINTS_SINGLE_COLOR = 6,
  DEBUG_POINTS_COLORS = 7,
  DEBUG_LINES_SINGLE_COLOR = 8,
  DEBUG_LINES_COLORS = 9,
  DEBUG_TRIANGLE_SINGLE_COLOR = 10,
  DEBUG_TRIANGLE_COLORS = 11,
  SKINCLUSTER = 12,
  SKINSKINCLUSTER = 13,
  FORWARD_PHONG_ALPHA_CUTOUT_SKIN = 14,
  HAIRSKIN = 15,
  FORWARD_PARALLAX = 16,
  SHADOW_SKIN_CLUSTER = 17
};

class MaterialManager final {
public:
  MaterialManager() : m_idxPool(RESERVE_SIZE) {
    m_materials.resize(RESERVE_SIZE), m_materialsMagic.resize(RESERVE_SIZE),
        m_materialTextureHandles.resize(RESERVE_SIZE),
        m_materialRuntimes.resize(RESERVE_SIZE);
  };
  ~MaterialManager() = default;
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
                    ID3D12GraphicsCommandList2 *commandList);
  void loadTypesInFolder(const char *folder);
  void bindRSandPSO(uint32_t shaderFlags,
                    ID3D12GraphicsCommandList2 *commandList);
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void init(){};
  MaterialHandle loadMaterial(const char *path, const SkinHandle handle);

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

  const std::string &getStringFromShaderTypeFlag(SHADER_TYPE_FLAGS type);

  const MaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandel(handle);
    return m_materialRuntimes[index];
  }
private:

  inline uint32_t getIndexFromHandel(const MaterialHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint16_t getMagicFromHandel(const MaterialHandle h) const {
    return static_cast<uint16_t>((h.handle & MAGIC_NUMBER_MASK) >> 16);
  }

  inline void assertMagicNumber(const MaterialHandle handle) {
    const uint16_t magic = getMagicFromHandel(handle);
    const uint32_t idx = getIndexFromHandel(handle);
    assert(m_materialsMagic[idx] == magic &&
           "invalid magic handle for constant buffer");
  }
  void loadTypeFile(const char *path);

private:
  std::unordered_map<uint16_t, ShaderBind> m_shderTypeToShaderBind;
  std::unordered_map<std::string, MaterialHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<uint32_t> m_idxPool;
  std::vector<Material> m_materials;
  std::vector<uint16_t> m_materialsMagic;
  std::vector<MaterialDataHandles> m_materialTextureHandles;
  std::vector<MaterialRuntime> m_materialRuntimes;
};

} // namespace SirEngine
