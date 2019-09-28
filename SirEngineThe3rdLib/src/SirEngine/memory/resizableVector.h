#pragma once
#include "SirEngine/memory/threeSizesPool.h"
#include <cassert>
#include <cstdint>

namespace SirEngine {

/*
This is a simple resizable vector which reflects the kind of usage I do in the
engine, not many features hopefully faster at both runtime(debug) and
compilation. You can only use POD data, and or structures in pod data, what
happens is a shallow copy when resizing or more memory required, if you have
pointers there those won't be deep copied, which might be the intended
behaviour, just bewhare!
*/
template <typename T, typename ALLOCATOR = ThreeSizesPool>
class ResizableVector {

public:
  explicit ResizableVector(const uint32_t reserveSize = 0,
                           ALLOCATOR *allocator = nullptr)
      : m_allocator(allocator) {
    m_size = 0;
    m_reserved = reserveSize;
    // allocate new memory if needed
    if (m_reserved != 0) {
      m_memory = reinterpret_cast<T *>(allocateMemoryInternal(m_reserved));
    }

#if SE_DEBUG
    // just setting memory to an easily readable value in case we are in debug
    memset(m_memory, 0xDEADBAAD, sizeof(T) * m_reserved);
#endif
  };

  ~ResizableVector() { freeMemoryInternal(m_memory); }

  inline void clear() { m_size = 0; }
  /*This function is designed  for quick removal of objects,
   *in this vector we cannot put object that need deep copy
   *as such we can shallow copy and move item easily, this method
   *will remove an object and copy the last one in its place of course
   *is not stable be careful of what you do! Speed comes with rules
   */

  T removeByPatchingFromLast(const uint32_t index) {
    assert(index < m_size);
    // we need to patch the index
    int copyIndex = m_size - 1;
    T value = m_memory[index];
    m_memory[index] = m_memory[copyIndex];
    --m_size;
    return value;
  }

  inline void pushBack(const T &value) {
    // first checking whether there is enough buffer left, if
    // not we re-allocate
    if (m_size >= m_reserved) {
      assert(m_reserved != 0);
      reallocateMemoryInternal(m_size * 2);
      m_reserved *= 2;
    }
    m_memory[m_size] = value;
    m_size += 1;
  };

  inline T &operator[](const uint32_t index) const {
#if SE_MEMORY_INDEX_CHECKING
    assert(index < m_size);
#endif
    return m_memory[index];
  }
  inline T &operator[](const int index) const {
#if SE_MEMORY_INDEX_CHECKING
    assert(index < m_size);
#endif
    return m_memory[index];
  }

  void resize(const uint32_t newSize) {
    if (newSize > m_reserved) {
      // if not enough space we re-allocate
      reallocateMemoryInternal(newSize * 2);
      m_reserved = newSize * 2;
      m_size = newSize;
    } else if (newSize < m_size) {
      // here we perform a trunctation
      m_size = newSize;
    }
  }

  inline const T &getConstRef(const uint32_t index) const {
#if SE_MEMORY_INDEX_CHECKING
    assert(index < m_size);
#endif
    return m_memory[index];
  }
  inline T *data() const { return m_memory; };
  inline uint32_t size() const { return m_size; }
  inline uint32_t reservedSize() const { return m_reserved; }

  // deleted functions
  ResizableVector(const ResizableVector &) = delete;
  ResizableVector &operator=(const ResizableVector &) = delete;

private:
  void *allocateMemoryInternal(uint32_t size, uint8_t = 0) {
    if (m_allocator != nullptr) {
      return (m_allocator->allocate(sizeof(T) * size));
    } else {
      return new T[size];
    }
  }
  void freeMemoryInternal(void *memory) {
    if ((m_allocator != nullptr) & (memory != nullptr)) {
      m_allocator->free(memory);
    } else {
      delete[] m_memory;
    }
  }

  void reallocateMemoryInternal(const uint32_t newSize) {
    T *tempMemory = reinterpret_cast<T *>(allocateMemoryInternal(newSize));
    if ((m_size != 0) & (m_memory != nullptr)) {
      memcpy(tempMemory, m_memory, m_size * sizeof(T));
    }
#if SE_DEBUG
    // just setting memory to an easily readable value in case we are in debug
    memset(tempMemory + m_size, 0xDEADBAAD, sizeof(T) * (newSize - m_size));
#endif
    freeMemoryInternal(m_memory);
    m_memory = tempMemory;
  }

private:
  ALLOCATOR *m_allocator;
  T *m_memory = nullptr;
  uint32_t m_size;
  uint32_t m_reserved;
}; // namespace SirEngine

} // namespace SirEngine