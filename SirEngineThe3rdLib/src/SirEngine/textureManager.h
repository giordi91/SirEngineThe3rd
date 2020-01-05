#pragma once

#include <unordered_map>

#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/handle.h"

namespace SirEngine {

// TODO can this be a enum class?

enum class RenderTargetFormat {
  RGBA32,
  R11G11B10_FLOAT,
  R11G11B10_UNORM,
  R16G16B16A16_FLOAT,
  BC1_UNORM,
  DEPTH_F32_S8
};

class TextureManager {
 public:
  enum TEXTURE_ALLOCATION_FLAG_BITS {
    ALLOW_RANDOM_WRITE = 1,
    DEPTH_TEXTURE = 2,
    RENDER_TARGET = 4,
    SHADER_RESOURCE = 8,
    COPY_SOURCE = 16,
    COPY_DEST = 32
  };
  typedef uint32_t TEXTURE_ALLOCATION_FLAGS;

 public:
  TextureManager() { m_nameToHandle.reserve(RESERVE_SIZE); }

  virtual ~TextureManager() = default;
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual TextureHandle loadTexture(const char *path, bool cubeMap = false) = 0;
  virtual void free(const TextureHandle handle) = 0;
  virtual TextureHandle allocateTexture(
      uint32_t width, uint32_t height, RenderTargetFormat format,
      const char *name, TEXTURE_ALLOCATION_FLAGS allocFlags,
      RESOURCE_STATE finalState = RESOURCE_STATE::RENDER_TARGET) = 0;

  virtual void bindRenderTarget(TextureHandle handle, TextureHandle depth) = 0;
  virtual void clearDepth(const TextureHandle depth, const float depthValue,
                          const float stencilValue) = 0;
  virtual void clearRT(const TextureHandle handle, const float color[4]) = 0;
  virtual TextureHandle getWhiteTexture() const = 0;

  inline TextureHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return TextureHandle{0};
  }

 protected:
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

}  // namespace SirEngine
