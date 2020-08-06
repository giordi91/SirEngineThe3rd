#pragma once
#include "SirEngine/handle.h"
#include "stdint.h"

namespace SirEngine {
struct GPUSlabAllocatorInitializeConfig {
  uint32_t slabSizeInBytes;
  uint32_t initialSlabs;
  bool allowNewSlabAllocations;
};
class GPUSlabAllocator {
 public:
  virtual ~GPUSlabAllocator() = default;
  virtual void initialize(GPUSlabAllocatorInitializeConfig config) = 0;
  virtual GPUSlabAllocationHandle allocate(uint32_t sizeInBytes,
                                           void *initialData = nullptr) = 0;
  virtual void *getMappedPtr(GPUSlabAllocationHandle allocHandle) = 0;
  // clearing the backing memory such that can be re-allocated, handles become
  // invalid
  virtual void clear() = 0;
  // freeing the actual backing memory
  virtual void cleanup() = 0;
  virtual uint32_t getAllocatedBytes(uint32_t slabIndex) const = 0;
  virtual uint32_t getSlabCount() const = 0;

  // we don't want to copy this allocator around
  GPUSlabAllocator() = default;
  GPUSlabAllocator(const GPUSlabAllocator &) = delete;
  GPUSlabAllocator &operator=(const GPUSlabAllocator &) = delete;
  GPUSlabAllocator(GPUSlabAllocator &&o) noexcept = delete;  // move constructor
  GPUSlabAllocator &operator=(GPUSlabAllocator &&other) =
      delete;  // move assignment
};

}  // namespace SirEngine
