#pragma once
#include <cassert>

// not thread safe
namespace SirEngine {

class StackAllocator final {
public:
  StackAllocator() = default;
  ~StackAllocator() {
	  delete[] m_start;
  }

  // request n bytes of memory
  void *allocate(const size_t sizeInByte) {
    assert(isAllocatorValid());
    auto basePtr = m_SP;
    m_SP += sizeInByte;
    assert(isAllocatorValid());
    return basePtr;
  };

  // free bits from the top of the stack
  void *free(const size_t sizeByte) {
    assert(isAllocatorValid());
    m_SP -= sizeByte;
    assert(isAllocatorValid());
    return m_SP;
  };

  void initialize(size_t sizeInByte) {
    assert(m_start == nullptr);
    assert(m_end == nullptr);
    m_start = new char[sizeInByte];
    m_SP = m_start;
    m_end = m_start + sizeInByte;
    assert(isAllocatorValid());
  };

  // this function  won't allocate anything but will get initialized
  // from a start and end and manange that memory, won't own it
  void setMemoryStartEnd(void *start, void *end) {
    assert(start != nullptr);
    assert(end != nullptr);
    assert(m_start == nullptr);
    assert(m_end == nullptr);
    assert((static_cast<char *>(end) > static_cast<char *>(start)));

    m_start = static_cast<char *>(start);
    m_end = static_cast<char *>(end);
    m_SP = m_start;

    assert(isAllocatorValid());
  };

  float getUsedMemoryPercentage() const {
    assert(isAllocatorValid());
    return (static_cast<float>(m_SP - m_start) /
            static_cast<float>(m_end - m_start));
  };

  inline void *getStartPtr() const { return m_start; }
  inline void *getStackPtr() const { return m_SP; }
  inline void *getEndPtr() const { return m_end; }

  // deleted copy constructor and assignment operator
  StackAllocator(StackAllocator const &) = delete;
  StackAllocator &operator=(StackAllocator const &) = delete;

private:
  bool isAllocatorValid() const {
    assert(m_start != nullptr);
    assert(m_end != nullptr);
    assert(m_SP != nullptr);
    assert((static_cast<char *>(m_end) > static_cast<char *>(m_start)));
    assert(m_SP < m_end);
    assert(m_SP >= m_start);
    return true;
  }

private:
  // member pointers
  char *m_SP{nullptr};
  char *m_start{nullptr};
  char *m_end{nullptr};
}; // namespace SirEngine

} // namespace SirEngine
