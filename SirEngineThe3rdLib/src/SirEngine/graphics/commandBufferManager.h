#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

class CommandBufferManager {
 public:
  virtual ~CommandBufferManager() = default;
  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual CommandBufferHandle createBuffer() = 0;
  virtual void executeBuffer(BufferHandle handle) = 0;
  virtual void resetBufferHandle(BufferHandle handle) = 0;
  virtual void executeFlushAndReset(BufferHandle handle) = 0;
  virtual void releaseBuffer(BufferHandle handle) = 0;
};

}  // namespace SirEngine
