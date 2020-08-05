#pragma once
#include "threeSizesPool.h"

namespace SirEngine {

template <typename T, int SMALL_SIZE_OPTIMIZATION = 10,
          typename ALLOCATOR = ThreeSizesPool>
class RingBuffer {
public:
  explicit RingBuffer(const int size, ALLOCATOR *alloc = nullptr) {

    assert(size > 0);
    m_size = size;
    // init membuffer
#ifdef _DEBUG
    memset(m_internalBuffer, 0xFF, sizeof(T) * SMALL_SIZE_OPTIMIZATION);
#else
    memset(m_internalBuffer, 0, sizeof(T) * SMALL_SIZE_OPTIMIZATION);
#endif

    m_alloc = alloc;
    if (size < SMALL_SIZE_OPTIMIZATION) {
      m_buffer = m_internalBuffer;
    } else {
      // buffer bigger than internal optimization needs to allocate
      if (m_alloc != nullptr) {
        m_buffer = reinterpret_cast<T *>(alloc->allocate(sizeof(T) * size));
      } else {
        m_buffer = new T[size];
      }
    }
  }

  ~RingBuffer() {
    if (!usesSmallSizeOptimization()) {
      if (m_alloc != nullptr) {
        m_alloc->free(m_buffer);
      } else {
        delete[] m_buffer;
      }
    }
  }

  bool push(T value) {
    // range check is not ideal, but I would rather have it than not, might be
    // disabled with a template parameter if becomes per critical code
    const bool canPush = (m_count) < m_size;
    if (!canPush) {
      return false;
    }
    // we can push
    int id = (m_start + m_count) % m_size;
    m_buffer[id] = value;
    ++m_count;
    return true;
  }

  T pop() {
    assert(m_count > 0);
    int idx = m_start;
    m_start = (m_start + 1) % m_size;
    --m_count;
    return m_buffer[idx];
  }
  const T &front() const { return m_buffer[m_start]; }

  inline int usedElementCount() const { return m_count; }
  bool isEmpty() const { return m_count == 0; }

  inline bool usesSmallSizeOptimization() const {
    return m_buffer == m_internalBuffer;
  }
  inline bool isFull() const { return m_count == m_size; }

private:
  ALLOCATOR *m_alloc = nullptr;
  T *m_buffer = nullptr;
  T m_internalBuffer[SMALL_SIZE_OPTIMIZATION];
  uint32_t m_start = 0;
  uint32_t m_count = 0;
  uint32_t m_size = 0;
};

} // namespace SirEngine
