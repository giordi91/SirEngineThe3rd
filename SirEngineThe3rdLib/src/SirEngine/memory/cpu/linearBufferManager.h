#pragma once
#include "SirEngine/memory/cpu/resizableVector.h"
#include <stdint.h>

namespace SirEngine {

struct BufferRangeHandle {
  uint32_t handle;
  [[nodiscard]] bool isHandleValid() const { return handle != 0; }
};

struct BufferRange {
  uint64_t m_offset;
  uint64_t m_size;

  [[nodiscard]] bool isValid() const { return (m_size != 0); }
};

struct BufferRangeTracker {
  BufferRange m_range;
  uint64_t m_actualAllocSize;
  uint16_t m_magicNumber;
  uint16_t m_allocIndex;
};

/*
 * This class is in charge to keep track of a buffer, it does not deal with
 * memory directly, it only keeps track that have been allocated or not. Not
 * dealing with memory makes it possible to deal with memory that is not
 * accessible, for example GPU ram, but from the CPU side you will be able to
 * know which part is free, usable , make sub allocations and so on.
 */
template class SIR_ENGINE_API ResizableVector<BufferRangeTracker>;
class SIR_ENGINE_API LinearBufferManager {
public:
  static constexpr uint32_t DEFAULT_ALLOCATION_RESERVE = 64;

public:
  explicit LinearBufferManager(
      const uint64_t bufferSizeInBytes,
      const uint32_t preAlloc = DEFAULT_ALLOCATION_RESERVE)
      : m_bufferSizeInBytes(bufferSizeInBytes), m_allocations(preAlloc),
        m_freeAllocations(preAlloc){};

  BufferRangeHandle allocate(const uint64_t allocSizeInBytes, const uint32_t alignment);
  void free(const BufferRangeHandle handle);

  // getters
  [[nodiscard]] uint64_t getBufferSizeInBytes() const {
    return m_bufferSizeInBytes;
  }
  [[nodiscard]] uint32_t getAllocationsCount() const { return m_allocCount; }
  [[nodiscard]] uint32_t getFreeAllocationsCount() const {
    return m_freeAllocations.size();
  }

  [[nodiscard]] const ResizableVector<BufferRangeTracker> *
  getAllocations() const {
    return &m_allocations;
  }

  [[nodiscard]] BufferRange
  getBufferRange(const BufferRangeHandle handle) const {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(idx < m_allocations.size());
    return m_allocations[idx].m_range;
  }

  [[nodiscard]] bool canAllocate(uint32_t allocSizeInBytes) const {
    uint64_t newStackPointer = allocSizeInBytes + m_stackPointer;
    bool allocationFits = newStackPointer < m_bufferSizeInBytes;
    if (allocationFits) {
      return true;
    }
    // if here alloc does not free need to search
    // let us first check if there are free allocations
    uint32_t count = m_freeAllocations.size();
    for (uint32_t i = 0; i < count; ++i) {
      BufferRangeTracker &range = m_freeAllocations[i];
      if (allocSizeInBytes <= range.m_actualAllocSize) {
        return true;
      }
    }
    return false;
  }

private:
  inline void assertMagicNumber(const BufferRangeHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    uint32_t storedMagic = m_allocations[idx].m_magicNumber;
    assert(storedMagic == magic && "invalid magic handle for buffer tracker");
  }

  static uint32_t getIndexFromHandle(const BufferRangeHandle h) {
    constexpr uint32_t STANDARD_INDEX_MASK = (1 << 16) - 1;
    return h.handle & STANDARD_INDEX_MASK;
  }

  static uint32_t getMagicFromHandle(const BufferRangeHandle h) {
    constexpr uint32_t STANDARD_INDEX_MASK = (1 << 16) - 1;
    const uint32_t STANDARD_MAGIC_NUMBER_MASK = ~STANDARD_INDEX_MASK;
    return (h.handle & STANDARD_MAGIC_NUMBER_MASK) >> 16;
  }

private:
  uint16_t MAGIC_NUMBER_COUNTER = 1;

  uint64_t m_bufferSizeInBytes;

  /*
   * Here we have two data structure that allow to keep track of what is free
   * and what is not. the main idea is that when an allocation is freed, the
   * m_allocations array is not resized, this allows us to keep the handles
   * indeces static, instead a copy is put in the free allocations, and when
   * that allocations is re-used then it will be put back in place at correct
   * index.
   */
  ResizableVector<BufferRangeTracker> m_allocations;
  ResizableVector<BufferRangeTracker> m_freeAllocations;
  uint64_t m_stackPointer = 0;
  uint32_t m_allocCount = 0;
};
} // namespace SirEngine