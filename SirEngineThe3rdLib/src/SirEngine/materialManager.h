#pragma once

#include "SirEngine/log.h"
// TODO texture manager should not be a dx12 one
#include "platform/windows/graphics/dx12/textureManager.h"

namespace SirEngine {

struct MaterialHandle final {
  uint32_t handle;
};

struct Material {
  float kDR;
  float kDG;
  float kDB;
  float kAR;
  float kAG;
  float kAB;
  float kSR;
  float kSG;
  float kSB;
};

struct MaterialCPU {
	//ConstantBufferHandle
  dx12::TextureHandle albedo;
  dx12::TextureHandle normal;
};

class MaterialManager final {

public:
  MaterialManager() = default;
  ~MaterialManager() = default;
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void initialize();
  void loadMaterial(const char *path);

private:
  std::unordered_map<std::string, MaterialHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialCPU> m_materialCPU;
  SparseMemoryPool<MaterialCPU> m_materialGPU;
};

} // namespace SirEngine
