#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {
class BufferManager {
public:
  BufferManager() = default;

  virtual ~BufferManager() = default;
  BufferManager(const BufferManager &) = delete;
  BufferManager &operator=(const BufferManager &) = delete;

  virtual void free(const BufferHandle handle) = 0;
  virtual BufferHandle allocate(const uint32_t sizeInByte,void *initData, const char *name,
                                 int numElements, int elementSize,bool isUAV) = 0;

  //virtual void bindBuffer(BufferHandle handle, int slot) = 0;
protected:
  inline uint32_t getIndexFromHandle(const BufferHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const BufferHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

protected:
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};
} // namespace SirEngine
