#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

class ConstantBufferManager {
public:
  enum CONSTANT_BUFFER_FLAGS {
    NONE = 0,
    UPDATED_EVERY_FRAME = 2,
  };

public:
  ConstantBufferManager() = default;
  virtual ~ConstantBufferManager() = default;
  ConstantBufferManager(const ConstantBufferManager &) = delete;
  ConstantBufferManager &operator=(const ConstantBufferManager &) = delete;

  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual bool free(ConstantBufferHandle handle) = 0;
  // TODO legacy call needs to be changed
  virtual ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes,
                                               void *data = nullptr) = 0;

  virtual ConstantBufferHandle
  allocate(uint32_t sizeInBytes, uint32_t flags = 0, void *data = nullptr) = 0;
  virtual void update(ConstantBufferHandle handle , void* data) =0;
  virtual void
  updateConstantBufferNotBuffered(const ConstantBufferHandle handle,
                                  void *dataToUpload) = 0;

  virtual void updateConstantBufferBuffered(const ConstantBufferHandle handle,
                                            void *dataToUpload) = 0;

  virtual void processBufferedData() = 0;

protected:
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace SirEngine
