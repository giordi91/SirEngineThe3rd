#pragma once
#include <cassert>

#include "SirEngine/globals.h"
#include "vector"
namespace SirEngine {

struct RandomSizeAllocationHandle {
  uint32_t offset = 0;
  uint16_t allocSize = 0;
  uint16_t dataSize = 0;
  inline bool isHandleValid() const { return allocSize > 0; }
};

class RandomSizeAllocator final {
  static const int DEBUG_VALUE = 0xBEEFBAAD;

 private:
  void set32BitMem(char *ptr, int sizeInBtye, int value) {
    int sizeInInt32 = sizeInBtye / 4;
    uint32_t *uintptr = reinterpret_cast<uint32_t *>(ptr);
    for (int i = 0; i < sizeInInt32; ++i) {
      *(uintptr + i) = value;
    }
  }

 public:
  RandomSizeAllocator() = default;
  void initialize(const uint32_t totalSizeInByte,
                  const int reservedAllocations = 20) {
    m_memory = new char[totalSizeInByte];
    m_unfragmentedPtr = m_memory;
    m_end = m_memory + totalSizeInByte;
    m_allocations.reserve(reservedAllocations);
#if SE_DEBUG
    set32BitMem(m_memory, totalSizeInByte, DEBUG_VALUE);
#endif
  }
  ~RandomSizeAllocator() { delete m_memory; }

  RandomSizeAllocationHandle allocate(const uint16_t sizeInByte) {
    // first inspect if we have any free allocation blocks
    RandomSizeAllocationHandle toReturnHandle;
    const int allocCount = static_cast<int>(m_allocations.size());
    const RandomSizeAllocationHandle *allocs = m_allocations.data();
    int foundIndex = -1;
    for (int i = 0; i < allocCount; ++i) {
      if (sizeInByte <= allocs[i].allocSize) {
        foundIndex = i;
        break;
      }
    }
    if (foundIndex != -1) {
      toReturnHandle = m_allocations[foundIndex];
      toReturnHandle.dataSize = sizeInByte;
      // patching the hole in the allocation list
      m_allocations[foundIndex] = m_allocations[m_allocations.size() - 1];
      m_allocations.pop_back();
    } else {
      // lets make a new allocation
      assert(m_unfragmentedPtr + sizeInByte < m_end);
      toReturnHandle.allocSize = sizeInByte;
      toReturnHandle.dataSize = sizeInByte;
      toReturnHandle.offset =
          static_cast<uint32_t>((m_unfragmentedPtr)-m_memory);
      // moving the unfrag pointer forward
      m_unfragmentedPtr += sizeInByte;
    }
    assert(toReturnHandle.isHandleValid());

#if SE_DEBUG
    assertMemoryIsNotAllocated(toReturnHandle);
#endif
    return toReturnHandle;
  }
  inline char *getPointer(const RandomSizeAllocationHandle handle) const {
    assert((m_memory + handle.offset + handle.allocSize) < m_end);
    return m_memory + handle.offset;
  }
  void freeAllocation(const RandomSizeAllocationHandle handle) {
    m_allocations.push_back(handle);
#if SE_DEBUG
    tagMemoryAsFreed(handle);
#endif
  }
  inline void tagMemoryAsFreed(const RandomSizeAllocationHandle handle) {
    char *ptr = getPointer(handle);
    assert((reinterpret_cast<int *>(ptr)[0] != DEBUG_VALUE));
    set32BitMem(ptr, handle.allocSize, DEBUG_VALUE);
  }

  inline const char *getStartPtr() const { return m_memory; };
  inline const char *getUnfragmentedPtr() const { return m_unfragmentedPtr; };

  inline void assertMemoryIsNotAllocated(
      const RandomSizeAllocationHandle handle) const {
    // simple check on the first int, might want to check the whole allocation
    // in the future although might be fairly heavy check
    char *ptr = getPointer(handle);
    assert((reinterpret_cast<int *>(ptr)[0] == static_cast<int>(DEBUG_VALUE)));
  }
  inline int getFreeBlocksCount() const {
    return static_cast<int>(m_allocations.size());
  }

  inline float getAllocatedAmount() const {
    auto range = static_cast<double>(m_end - m_memory);
    auto curr = static_cast<double>(m_unfragmentedPtr - m_memory);
    return static_cast<float>(curr / range);
  }

 private:
  char *m_memory = nullptr;
  char *m_unfragmentedPtr = nullptr;
  char *m_end = nullptr;
  std::vector<RandomSizeAllocationHandle> m_allocations;
};
}  // namespace SirEngine
