#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace SirEngine {

template <size_t SMALL_SIZE, size_t MEDIUM_SIZE>
class ThreeSizesPool final {
 private:
// flags used to mark the headers and allocations
#define SMALL_ALLOC_TAG 1;
#define MEDIUM_ALLOC_TAG 2;
#define LARGE_ALLOC_TAG 4;

  struct NextAlloc {
    uint32_t offset : 32;  // offset from the start of the pool in byte
    uint32_t nextOffset;   // offset from the start of the pool in byte for the
                           // next node in the linked list, 0 if null
    bool isNode : 1;     // a bit flag setting whether the data here refers to a
                         // node or an alloc, mostly debugging purpose
    uint32_t size : 28;  // size of the allocation in byte
    uint32_t allocType : 3;  // type of allocation
  };

  // this is an allocation description, is always going to be present, so if we
  // ask to allocate a some memory we will always allocate that memory + the
  // header
  struct AllocHeader {
    uint32_t size : 28;
    uint32_t type : 3;
    uint32_t isNode : 1;
  };

 public:
  explicit ThreeSizesPool(const uint32_t poolSizeInByte) {
    m_poolSizeInByte = poolSizeInByte;
    m_memory = new char[m_poolSizeInByte];

    m_nextAlloc[0] = nullptr;
    m_nextAlloc[1] = nullptr;
    m_nextAlloc[2] = nullptr;
  };

  ~ThreeSizesPool() { delete m_memory; }

  int allocationInPool(const char *ptr) const {
    const int delta = ptr - m_memory;
    return (delta > 0) & (delta < m_poolSizeInByte);
  }

  uint32_t getAllocSize(void *memoryPtr) const {
    return getRawAllocSize(memoryPtr) - sizeof(AllocHeader);
  }

  uint32_t getRawAllocSize(void *memoryPtr) const {
    char *bytePtr = reinterpret_cast<char *>(memoryPtr);
    assert(allocationInPool(bytePtr));

    AllocHeader *header =
        reinterpret_cast<AllocHeader *>(bytePtr - sizeof(AllocHeader));
    assert(header->isNode == 0);
    return header->size;
  }

  static uint32_t getMinAllocSize() { return MIN_ALLOC_SIZE; }

  ThreeSizesPool(const ThreeSizesPool &) = delete;
  ThreeSizesPool &operator=(const ThreeSizesPool &) = delete;

  static uint32_t getAllocationTypeFromSize(const uint32_t sizeInByte) {
    const int isInMediumRange =
        (sizeInByte < MEDIUM_SIZE) & (sizeInByte >= SMALL_SIZE);
    const int isInLargeRange = sizeInByte >= MEDIUM_SIZE;

    return isInMediumRange + isInLargeRange * 2;
  }

  void *allocateNew(const uint32_t sizeInByte) {
    uint32_t totalAllocSize = sizeInByte + sizeof(AllocHeader);
	totalAllocSize  = totalAllocSize < MIN_ALLOC_SIZE? MIN_ALLOC_SIZE : totalAllocSize;

    auto *header =
        reinterpret_cast<AllocHeader *>(m_memory + m_stackPointerOffset);
    header->size = totalAllocSize;
    header->isNode = false;
    header->type = getAllocationTypeFromSize(sizeInByte);

    ++m_allocCount[header->type];
    m_stackPointerOffset += totalAllocSize;

    return reinterpret_cast<char *>(header) + sizeof(AllocHeader);
  }

  void *allocate(const uint32_t sizeInByte) {
    uint32_t allocType = getAllocationTypeFromSize(sizeInByte);
    NextAlloc *next = m_nextAlloc[allocType];
    if (next != nullptr) {
      // ok we have something in the cache let us try see if one slot is big
      // enough
      const uint32_t totalAllocSize = sizeInByte + sizeof(AllocHeader);
      NextAlloc *parent = nullptr;
      NextAlloc *found =
          findNextFreeAllocForSize(next, totalAllocSize, &parent);
      if (found == nullptr) {
        return allocateNew(sizeInByte);
      }
    }

    // no hit in the cache, we need to allocate from the pool
    return allocateNew(sizeInByte);
  }

  NextAlloc *findNextFreeAllocForSize(NextAlloc *start, uint32_t totalAllocSize,
                                      NextAlloc **parent) {
    if (totalAllocSize <= start->size) {
      return start;
    }
    NextAlloc *current = start;
    while (current->nextOffset != 0) {
      current = reinterpret_cast<NextAlloc *>(m_memory + current->nextOffset);
      if (totalAllocSize < current->size) {
        return current;
      }
    }
    return nullptr;
  }


  uint32_t getSmallAllocCount() const { return m_allocCount[0]; }
  uint32_t getMediumAllocCount() const { return m_allocCount[1]; }
  uint32_t getLargeAllocCount() const { return m_allocCount[2]; }

 private:
  static constexpr uint32_t MIN_ALLOC_SIZE = sizeof(NextAlloc);
  char *m_memory = nullptr;
  uint32_t m_poolSizeInByte;
  uint32_t m_stackPointerOffset = 0;

  NextAlloc *m_nextAlloc[3];
  uint32_t m_allocCount[3]{};

#if SE_DEBUG
  char *m_freedMemory = nullptr;
#endif
};

}  // namespace SirEngine
