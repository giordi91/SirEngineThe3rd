#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

class ConstantBufferManager{
public:
  ConstantBufferManager() =default;
  virtual ~ConstantBufferManager() = default;
  ConstantBufferManager(const ConstantBufferManager&) = delete;
  ConstantBufferManager&
  operator=(const ConstantBufferManager&) = delete;

  virtual ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes, void* data = nullptr) =0;
  virtual void updateConstantBufferNotBuffered(const ConstantBufferHandle handle,
                                    void* dataToUpload) =0;

  virtual void updateConstantBufferBuffered(const ConstantBufferHandle handle,
                                    void* dataToUpload) =0;

protected:
  inline uint32_t getIndexFromHandle(const ConstantBufferHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint16_t getMagicFromHandle(const ConstantBufferHandle h) const {
    return static_cast<uint16_t>((h.handle & MAGIC_NUMBER_MASK) >> 16);
  }

protected:
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace SirEngine
