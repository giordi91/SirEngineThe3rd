#pragma once

#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include <cassert>

#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {

struct Dx12MaterialRuntime final {
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
  MeshHandle meshHandle;
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

struct MaterialData {
  MaterialDataHandles handles;
  dx12::DescriptorPair albedoSrv;
  dx12::DescriptorPair normalSrv;
  dx12::DescriptorPair metallicSrv;
  dx12::DescriptorPair roughnessSrv;
  dx12::DescriptorPair thicknessSrv;
  dx12::DescriptorPair separateAlphaSrv;
  dx12::DescriptorPair aoSrv;
  dx12::DescriptorPair heightSrv;
  uint32_t magicNumber;
  Material m_material;
  Dx12MaterialRuntime m_materialRuntime;
};

class Dx12MaterialManager final : public MaterialManager {
public:
  Dx12MaterialManager()
      : MaterialManager(), m_shderTypeToShaderBind(RESERVE_SIZE),
        m_nameToHandle(RESERVE_SIZE), m_materialTextureHandles(RESERVE_SIZE){};
  ~Dx12MaterialManager() = default;
  void inititialize() override{};
  void cleanup() override{};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
                    ID3D12GraphicsCommandList2 *commandList);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const Dx12MaterialRuntime &runtime,
                    ID3D12GraphicsCommandList2 *commandList);

  void loadTypesInFolder(const char *folder);
  void bindRSandPSO(uint32_t shaderFlags,
                    ID3D12GraphicsCommandList2 *commandList);
  Dx12MaterialManager(const Dx12MaterialManager &) = delete;
  Dx12MaterialManager &operator=(const Dx12MaterialManager &) = delete;

  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle);

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

  const char *getStringFromShaderTypeFlag(SHADER_TYPE_FLAGS type);

  const Dx12MaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }

private:
  inline void assertMagicNumber(const MaterialHandle handle) {
    const uint16_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }
  void loadTypeFile(const char *path);

private:
  // TODO let's remove this std maps
  // std::unordered_map<uint16_t, ShaderBind> m_shderTypeToShaderBind;
  // std::unordered_map<std::string, MaterialHandle> m_nameToHandle;
  HashMap<uint16_t, ShaderBind, hashUint16> m_shderTypeToShaderBind;
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
};

} // namespace SirEngine
