#pragma once
#include <stdint.h>

namespace SirEngine {

struct MaterialHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct TextureHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};
struct PSOHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct ConstantBufferHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};
struct MeshHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct AssetHandles {
  MeshHandle meshH;
  MaterialHandle materialH;
};
} // namespace SirEngine
