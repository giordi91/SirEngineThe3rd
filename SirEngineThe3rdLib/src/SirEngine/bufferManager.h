#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {

class BufferManager {
 public:
  enum BUFFER_FLAGS_BITS {
    RANDOM_WRITE = 1 << 0,
    INDEX_BUFFER = 1 << 1,
    INDIRECT_BUFFER = 1 << 2,
    VERTEX_BUFFER = 1 << 3,
    BUFFERED = 1 << 4,
    STORAGE_BUFFER = 1 << 5,
    GPU_ONLY = 1 << 6,
    IS_STATIC = 1 << 7,
  };
  typedef uint32_t BUFFER_FLAGS;

  enum BUFFER_BARRIER_STATE_BITS {
    BUFFER_STATE_NONE = 0,
    BUFFER_STATE_WRITE = 1,
    BUFFER_STATE_READ = 2,
    BUFFER_STATE_INDIRECT_DRAW = 3,
  };
  typedef uint32_t BUFFER_BARRIER_STATE;

  enum BUFFER_BARRIER_STAGE_BITS {
    BUFFER_STAGE_NONE = 0,
    BUFFER_STAGE_GRAPHICS = 1,
    BUFFER_STAGE_COMPUTE = 2,
    BUFFER_STAGE_INDIRECT_DRAW = 3,
  };
  typedef uint32_t BUFFER_BARRIER_STAGE;

  struct BufferTransition {
    BUFFER_BARRIER_STATE sourceState;
    BUFFER_BARRIER_STATE destinationState;
    BUFFER_BARRIER_STAGE sourceStage;
    BUFFER_BARRIER_STAGE destinationStage;
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
                                int elementSize, BUFFER_FLAGS flags) = 0;
  virtual void transitionBuffer(const BufferHandle handle,
                                const BufferTransition &transition) = 0;
  virtual void *getMappedData(const BufferHandle handle) const = 0;
  virtual void update(BufferHandle handle, void *inData, int offset,
                      int size) = 0;
};

}  // namespace SirEngine
