#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {

class BufferManager {
 public:
  enum BUFFER_FLAGS_BITS {
    RANDOM_WRITE = 1,
    INDEX_BUFFER = 2,
    INDIRECT_BUFFER = 4,
    VERTEX_BUFFER = 8,
    BUFFERED = 16,
    STORAGE_BUFFER = 32,
    GPU_ONLY = 64,
  };

  typedef uint32_t BUFFER_FLAGS;

 public:
  BufferManager() = default;

  virtual ~BufferManager() = default;
  BufferManager(const BufferManager &) = delete;
  BufferManager &operator=(const BufferManager &) = delete;

  virtual void initialize() = 0;
  virtual void cleanup() = 0;

  virtual void free(const BufferHandle handle) = 0;
  virtual BufferHandle allocate(const uint32_t sizeInBytes, void *initData,
                                const char *name, int numElements,
                                int elementSize, BUFFER_FLAGS flags) = 0;
  virtual void *getMappedData(const BufferHandle handle) const = 0;
};

}  // namespace SirEngine
