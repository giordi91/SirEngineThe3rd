#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace SirEngine {

template <typename T> class SparseMemoryPool final {

public:
  explicit SparseMemoryPool(const uint32_t poolSize) {
    m_memory = new T[poolSize];
    m_poolSize = poolSize;
    m_freeList = new uint32_t[poolSize];

#if SE_DEBUG
    m_freedMemory = new char[poolSize];
    memset(m_freedMemory, 0, sizeof(char) * poolSize);
#endif
  };

  ~SparseMemoryPool() {
    delete m_memory;
    delete m_freeList;
#if SE_DEBUG
    delete m_freedMemory;
#endif
  };
  SparseMemoryPool(const SparseMemoryPool &) = delete;
  SparseMemoryPool &operator=(const SparseMemoryPool &) = delete;

  inline T &getFreeMemoryData(uint32_t &index) {
    assert(m_allocationCount < m_poolSize &&
           "requested deallocation is outside pool range");

    // if there is not free index
    if (m_freeListCount == 0) {
      index = m_allocationCount++;
      m_memory[index] = T{};
      return m_memory[index];
    } else {
      // lets re-use the slot
      index = m_freeList[m_freeListCount - 1];
      T &data = m_memory[index];
      --m_freeListCount;
#if SE_DEBUG
      assert(m_freedMemory[index] == 1);
      m_freedMemory[index] = 0;
#endif
      return data;
    }
  }
  inline void free(const uint32_t index) {
    assert(m_allocationCount < m_poolSize &&
           "requested deallocation is outside pool range");
    assert(m_freedMemory[index] == 0 && "memory has been already deallocated");

    m_freeList[m_freeListCount++] = index;
    --m_allocationCount;

#if SE_DEBUG
    m_freedMemory[index] = 1;
#endif
  }
  inline uint32_t getAllocatedCount() const { return m_allocationCount; }

private:
  T *m_memory = nullptr;
  uint32_t *m_freeList = nullptr;
  uint32_t m_poolSize;
  uint32_t m_freeListCount = 0;
  uint32_t m_allocationCount = 0;
#if SE_DEBUG
  char *m_freedMemory = nullptr;
#endif
};

} // namespace SirEngine
