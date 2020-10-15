#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

class CommandBufferManager {
 public:
  enum COMMAND_BUFFER_ALLOCATION_BITS {
    COMMAND_BUFFER_ALLOCATION_NONE = 0,
  };
  typedef uint32_t COMMAND_BUFFER_ALLOCATION_FLAGS;

 public:
  virtual ~CommandBufferManager() = default;
  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual CommandBufferHandle createBuffer(
      COMMAND_BUFFER_ALLOCATION_FLAGS flags, const char* name = nullptr) = 0;
  virtual void executeBuffer(CommandBufferHandle handle) = 0;
  virtual void freeBuffer(CommandBufferHandle handle) = 0;
  virtual void resetBufferHandle(CommandBufferHandle handle) = 0;
  virtual void flush(CommandBufferHandle handle) = 0;
  virtual void executeFlushAndReset(CommandBufferHandle handle) = 0;
};

}  // namespace SirEngine
