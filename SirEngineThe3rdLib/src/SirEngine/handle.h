#pragma once
#include "SirEngine/core.h"

namespace SirEngine {

template <typename T>
inline uint32_t getIndexFromHandle(const T h) {
  constexpr uint32_t standardIndexMask = (1 << 16) - 1;
  return h.handle & standardIndexMask;
}
template <typename T>
inline uint32_t getMagicFromHandle(const T h) {
  constexpr uint32_t standardIndexMask = (1 << 16) - 1;
  const uint32_t standardMagicNumberMask = ~standardIndexMask;
  return (h.handle & standardMagicNumberMask) >> 16;
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

struct BindingTableHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct LightHandle {
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

struct DescriptorHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct BufferBindingsHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct GPUSlabAllocationHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct CommandBufferHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

}  // namespace SirEngine
