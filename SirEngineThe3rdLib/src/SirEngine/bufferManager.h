#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {

class BufferManager {
public:
  enum BUFFER_FLAGS {
    RANDOM_WRITE = 1,
    INDEX_BUFFER = 2,
    INDIRECT_BUFFER = 4,
    VERTEX_BUFFER = 8,
    BUFFERED = 16,
    UPDATED_EVERY_FRAME =32 
  };

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
                                int elementSize, uint32_t flags) = 0;
  virtual BufferHandle allocateUpload(const uint32_t sizeInByte,
                                      const char *name) = 0;

};

} // namespace SirEngine
