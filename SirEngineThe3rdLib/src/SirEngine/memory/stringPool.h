#pragma once
#include "SirEngine/core.h"

#include "SirEngine/memory/stackAllocator.h"
#include "SirEngine/memory/threeSizesPool.h"

namespace SirEngine {

enum STRING_MANIPULATION_FLAGS {
  NONE = 0,
  FREE_FIRST_AFTER_OPERATION = 1 << 1,
  FREE_SECOND_AFTER_OPERATION = 1 << 2,
  FREE_JOINER_AFTER_OPERATION = 1 << 3
};
// NOTE: this class does not handle a string starting with leading spaces
class SIR_ENGINE_API StringPool final {
 public:
 public:
  explicit StringPool(const uint32_t sizeInByte) : m_pool(sizeInByte) {
    m_stackAllocator.initialize(sizeInByte);
  };
  // deleted copy constructors and assignment operator
  StringPool(const StringPool&) = delete;
  StringPool& operator=(const StringPool&) = delete;

  // allocations and free
  const char* allocateStatic(const char* string);
  const wchar_t* allocateStatic(const wchar_t* string);
  const char* allocateFrame(const char* string);
  const wchar_t* allocateFrame(const wchar_t* string);

  inline void free(const char* string) { m_pool.free((void*)string); }
  inline void free(const wchar_t* string) { m_pool.free((void*)string); };
  inline void resetFrameMemory() { m_stackAllocator.reset(); }

  // string manipulation
  const char* concatenateStatic(const char* first, const char* second,
                                const char* joiner = nullptr,
                                uint8_t flags = 0);

 private:
  enum class STRING_TYPE { CHAR = 1, WIDECHAR = 2 };

 private:
  uint32_t getStaticLength(const char* string, bool isInPool) const;

 private:
  ThreeSizesPool<64, 256> m_pool;
  StackAllocator m_stackAllocator;
};

}  // namespace SirEngine
