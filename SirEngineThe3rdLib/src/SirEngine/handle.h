#pragma once
#include <stdint.h>

namespace SirEngine
{
struct MaterialHandle final {
  uint32_t handle;
};

struct TextureHandle final {
  uint32_t handle;
};

struct ConstantBufferHandle final {
  uint32_t handle;
};
}