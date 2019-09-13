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
struct RSHandle final {
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
struct BufferHandle final {
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct StreamHandle { 
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct DebugDrawHandle{ 
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct AnimationConfigHandle{ 
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};

struct SkinHandle{ 
  uint32_t handle;
  bool isHandleValid() const { return handle != 0; }
};
} // namespace SirEngine
