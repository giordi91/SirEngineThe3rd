#pragma once
#include "threeSizesPool.h"

namespace SirEngine {

template <typename T, typename ALLOCATOR = ThreeSizesPool>
class OverridingRingBuffer {
  enum RING_CONTROL_FLAGS : uint8_t { FREE = 0, USED = 1 };

 public:
  explicit OverridingRingBuffer(const int size, ALLOCATOR *alloc = nullptr) {
    assert(size > 0);
    m_size = size;
    // init membuffer

    m_alloc = alloc;
    // buffer bigger than internal optimization needs to allocate
    if (m_alloc != nullptr) {
      m_buffer = reinterpret_cast<T *>(alloc->allocate(sizeof(T) * size));
      m_controlBuffer =
          reinterpret_cast<uint8_t *>(alloc->allocate(sizeof(uint8_t) * size));
    } else {
      m_buffer = new T[size];
      m_controlBuffer = new uint8_t[size];
    }
    // clearing the control buffer
    memset(m_controlBuffer, RING_CONTROL_FLAGS::FREE, sizeof(char) * size);
  }
  inline void registerDestroyCallback(void (*callback)(T &)) {
    m_callback = callback;
  }

  ~OverridingRingBuffer() {
    // freeing whatever needed
    if (m_callback != nullptr) {
      for (uint32_t i = m_head; i < m_count; ++i) {
        uint32_t idx = i % m_size;
        assert(m_controlBuffer[idx] == USED);
        if (m_controlBuffer[idx] != FREE) {
          m_callback(m_buffer[idx]);
        }
      }
    }
    if (m_alloc != nullptr) {
      m_alloc->free(m_buffer);
    } else {
      delete[] m_buffer;
    }
  }

  bool push(T value) {
    int id = m_head;
    m_head = (m_head + 1) % m_size;
    // if the slot is occupied and wee have a callback we want to free the
    // element properly
    if ((m_controlBuffer[id] != FREE) & (m_callback != nullptr)) {
      m_callback(m_buffer[id]);
    }
    m_buffer[id] = value;
    m_count += m_controlBuffer[id] == FREE ? 1 : 0;
    m_controlBuffer[id] = USED;
    return true;
  }

  T pop() {
    assert(m_count > 0);
    uint32_t idx = getFirstElementIndex();
    assert(m_controlBuffer[idx] == USED);
    m_controlBuffer[idx] = FREE;
    // we don't de-alloc we return it to the user is up to him to do dealloc if
    // requested
    --m_count;
    return m_buffer[idx];
  }
  T back() const {
    uint32_t idx = getLastElementIndex();
    return m_buffer[idx];
  }

  const T &front() const {
    uint32_t idx = getFirstElementIndex();
    return m_buffer[idx];
  }

  const T *getData() const { return m_buffer; }
  T *getData() { return m_buffer; }
  inline int usedElementCount() const { return m_count; }
  inline int getSize() const { return m_size; }
  bool isEmpty() const { return m_count == 0; }

  inline bool isFull() const { return m_count == m_size; }

  uint32_t getFirstElementIndex() const {
    // this is the index if is full
    int idxSigned = static_cast<int>(m_head);
    idxSigned -= m_count;
    idxSigned += idxSigned < 0 ? m_size : 0;
    assert((idxSigned >= 0) & (static_cast<uint32_t>(idxSigned) < m_size));
    auto idxFull = static_cast<uint32_t>(idxSigned);
    return idxFull;
  }
  uint32_t getLastElementIndex() const {
    int idxSigned = static_cast<int>(m_head);
    idxSigned -= 1;
    idxSigned += idxSigned < 0 ? m_size : 0;
    assert((idxSigned >= 0) & (static_cast<uint32_t>(idxSigned) < m_size));
    auto idxFull = static_cast<uint32_t>(idxSigned);
    return idxFull;
  }

  void clear() {
    uint32_t count = m_count;
    for (uint32_t i = 0; i < count; ++i) {
      T value = pop();
      if (m_callback != nullptr) {
        m_callback(value);
      }
    }
  }

 private:
  ALLOCATOR *m_alloc = nullptr;
  T *m_buffer = nullptr;
  uint8_t *m_controlBuffer = nullptr;
  uint32_t m_head = 0;  // next free slot
  uint32_t m_count = 0;
  uint32_t m_size = 0;
  void (*m_callback)(T &) = nullptr;
};

}  // namespace SirEngine
