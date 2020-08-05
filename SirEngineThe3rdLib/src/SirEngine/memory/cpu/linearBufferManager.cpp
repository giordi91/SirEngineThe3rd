#include "SirEngine/memory/cpu/linearBufferManager.h"
#include <memory>

namespace SirEngine {

uint64_t alignTo(const uint64_t stackOffset, const uint32_t alignment) {
  const bool isAligned = stackOffset % alignment == 0;
  return isAligned ? stackOffset : ((stackOffset / alignment) + 1) * alignment;
}

BufferRangeHandle LinearBufferManager::allocate(const uint64_t allocSizeInBytes,
                                                const uint32_t alignment) {
  const uint64_t alignedStackPointer = alignTo(m_stackPointer, alignment);
  const uint64_t newStackPointer = allocSizeInBytes + alignedStackPointer;
  const bool allocationFits = newStackPointer < m_bufferSizeInBytes;
  const bool hasFreeAllocations = m_freeAllocations.size() != 0;
  if ((!allocationFits) & (!hasFreeAllocations)) {
    // allocation does not fit and no free allocations
    return BufferRangeHandle{};
  }

  // let us first check if there are free allocations
  uint32_t count = m_freeAllocations.size();
  ;
  for (uint32_t i = 0; i < count; ++i) {
    BufferRangeTracker &range = m_freeAllocations[i];
    if (allocSizeInBytes > range.m_actualAllocSize) {
      continue;
    }

    // if we are here means the allocation could fit!
    range.m_range.m_size = allocSizeInBytes;
    range.m_magicNumber = MAGIC_NUMBER_COUNTER++;
    range.m_allocIndex = i;
    // we need to move it back into the free
    m_allocations[range.m_allocIndex] = range;
    m_freeAllocations.removeByPatchingFromLast(i);
    // lets return the handle
    const BufferRangeHandle handle{static_cast<uint32_t>(
        (range.m_magicNumber << 16u) | range.m_allocIndex)};
    m_allocCount += 1;
    return handle;
  }

  // if we are here we need a new allocation
  BufferRangeTracker toReturn{};
  toReturn.m_range = {alignedStackPointer, allocSizeInBytes};
  toReturn.m_allocIndex = static_cast<uint16_t>(m_allocations.size());
  toReturn.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  toReturn.m_actualAllocSize = allocSizeInBytes;
  m_allocations.pushBack(toReturn);

  // moving stack pointer up
  m_stackPointer = alignedStackPointer + allocSizeInBytes;
  m_allocCount += 1;

  const BufferRangeHandle handle{static_cast<uint32_t>(
      (toReturn.m_magicNumber << 16) | toReturn.m_allocIndex)};

  return handle;
}

void LinearBufferManager::free(const BufferRangeHandle handle) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  assert(idx < m_allocations.size());

  BufferRangeTracker &tracker = m_allocations[idx];
  // this invalidate the tracker
  tracker.m_range.m_size = 0;
  assert(tracker.m_range.isValid() == false);
  m_freeAllocations.pushBack(tracker);
  m_allocCount -= 1;
}

} // namespace SirEngine