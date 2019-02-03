#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace SirEngine {

// The Sparse in the names stands from the fact that, although
// the pool tries to patch holes on new allocation, there is no
// actual hard guarantees that the memory will actually be contiguous

// the way it works is the following, you have a memory pool, which gets
// allocated at the beginning. The memory gets initialized by a linked list,
// which is a fancy term for index of the next available slot.
// this mean the Nth index array will be initialized from Nth+1,
// for example starting from zero memory will be set as 1,2,3,4...

// the next empty slots starts at zero, when an allocation is made,
// the nextAllocation slot is used and the value at which the slot points
// to will be set as nextAllocation, for example if you do your first allocation
// the address of memory[0] will be returned and nextAllocation will be 1,
// because that is the value that memory[0] holds at the beginning.

// deletion works in a similar fashion, once a slot is freed, in the current
// freed slot we store the current nextAllocation slot, and nextAllocation slot
// gets set to the newly freed index.

template <typename T> class SparseMemoryPool final {

public:
  explicit SparseMemoryPool(const uint32_t poolSize) {
    // since we use uint32_t as indices we need at least 4 byte dataType
    // to store the "linked list" in the same memory.
    assert(sizeof(T) >= 4);

    m_memory = new T[poolSize];
    m_poolSize = poolSize;
    // m_freeList = new uint32_t[poolSize];
    // initializing the "linked list", nextAlloc is already init to 0;
    for (uint32_t i = 0; i < poolSize; ++i) {
      *(reinterpret_cast<uint32_t *>(&m_memory[i])) = i + 1;
    }

#if SE_DEBUG
    m_freedMemory = new char[poolSize];
    memset(m_freedMemory, 1, sizeof(char) * poolSize);
#endif
  };

  ~SparseMemoryPool() {
    delete m_memory;
#if SE_DEBUG
    delete m_freedMemory;
#endif
  };
  SparseMemoryPool(const SparseMemoryPool &) = delete;
  SparseMemoryPool &operator=(const SparseMemoryPool &) = delete;

  inline T &getFreeMemoryData(uint32_t &index) {
    assert(m_allocationCount < m_poolSize &&
           "requested deallocation is outside pool range");

    index = m_nextAllocation;
    m_nextAllocation =
        *(reinterpret_cast<uint32_t *>(&m_memory[m_nextAllocation]));
    ++m_allocationCount;
#if SE_DEBUG
    m_memory[index] = T{};
    assert(m_freedMemory[index] == 1);
    m_freedMemory[index] = 0;
#endif
    return m_memory[index];
  }
  inline void free(const uint32_t index) {
    assert(m_allocationCount < m_poolSize &&
           "requested deallocation is outside pool range");
    assert(m_freedMemory[index] == 0 && "memory has been already deallocated");
    --m_allocationCount;

#if SE_DEBUG
    m_freedMemory[index] = 1;
    m_memory[index] = T{};
#endif
    // set in the new freed slot the value to the next free slot
    *(reinterpret_cast<uint32_t *>(&m_memory[index])) = m_nextAllocation;
    m_nextAllocation = index;
  }
  inline uint32_t getAllocatedCount() const { return m_allocationCount; }

  // subscript operator to access the pool directly, we are adults, we don't
  // make mistakes, direct memory access is fine.
  inline T &operator[](const uint32_t index) {
    assert(index < m_allocationCount);
    return m_memory[index];
  }

  inline const T &getConstRef(const uint32_t index) const {
    assert(index < m_allocationCount);
    return m_memory[index];
  }

private:
  T *m_memory = nullptr;
  uint32_t m_poolSize;
  uint32_t m_allocationCount = 0;
  uint32_t m_nextAllocation = 0;
#if SE_DEBUG
  char *m_freedMemory = nullptr;
#endif
};

} // namespace SirEngine
