#pragma once
#include <stdint.h>

namespace SirEngine {

template <typename T> inline uint32_t getIndexFromHandle(const T h) {
  constexpr uint32_t STANDARD_INDEX_MASK = (1 << 16) - 1;
  return h.handle & STANDARD_INDEX_MASK;
}
template <typename T> inline uint32_t getMagicFromHandle(const T h) {
  constexpr uint32_t STANDARD_INDEX_MASK = (1 << 16) - 1;
  const uint32_t STANDARD_MAGIC_NUMBER_MASK = ~STANDARD_INDEX_MASK;
  return (h.handle & STANDARD_MAGIC_NUMBER_MASK) >> 16;
}

struct AssetDataHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct MaterialHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct TextureHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};
struct PSOHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};
struct RSHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct ConstantBufferHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};
struct MeshHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};
struct BufferHandle final {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct StreamHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct DebugDrawHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct AnimationConfigHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct SkinHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct ScriptHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};


} // namespace SirEngine
