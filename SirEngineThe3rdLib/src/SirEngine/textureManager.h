#pragma once

#include "SirEngine/handle.h"
#include <unordered_map>

namespace SirEngine {

enum TextureFlags { DEPTH = 1, RT = 2 };

enum class RenderTargetFormat {
  RGBA32,
  R11G11B10_FLOAT,
  R11G11B10_UNORM,
  R16G16B16A16_FLOAT
};
class TextureManager {
public:
  TextureManager() { m_nameToHandle.reserve(RESERVE_SIZE); }

  virtual ~TextureManager() = default;
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;
  virtual TextureHandle loadTexture(const char *path, bool cubeMap = false) = 0;
  virtual void free(const TextureHandle handle) = 0;
  virtual TextureHandle allocateRenderTexture(uint32_t width, uint32_t height,
                                              RenderTargetFormat format,
                                              const char *name,
                                              bool allowWrite = false) = 0;
  virtual void copyTexture(TextureHandle source, TextureHandle destination) = 0;
  virtual void bindRenderTarget(TextureHandle handle, TextureHandle depth) = 0;
  virtual void bindBackBuffer(bool bindBackBufferDepth) = 0;
  virtual void clearDepth(const TextureHandle depth, float value = 1.0f) = 0;
  virtual void clearRT(const TextureHandle handle, const float color[4]) = 0;

  inline TextureHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return TextureHandle{0};
  }

protected:
  inline uint32_t getIndexFromHandle(const TextureHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const TextureHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

protected:
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace SirEngine
