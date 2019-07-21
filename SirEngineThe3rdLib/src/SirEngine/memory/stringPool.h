#pragma once
#include "SirEngine/core.h"

#include "SirEngine/memory/stackAllocator.h"
#include "SirEngine/memory/threeSizesPool.h"

namespace SirEngine {

class SIR_ENGINE_API StringPool final {
 public:
  enum class STRING_TYPE { CHAR = 1, WIDECHAR = 2 };

 public:
  explicit StringPool(const uint32_t sizeInByte) : m_pool(sizeInByte) {
    m_stackAllocator.initialize(sizeInByte);
  };
  // deleted copy constructors and assignment operator
  StringPool(const StringPool&) = delete;
  StringPool& operator=(const StringPool&) = delete;

  const char* allocateStatic(const char* string);

  const wchar_t* allocateStatic(const wchar_t* string);

  const char* allocateFrame(const char* string);

  const wchar_t* allocateFrame(const wchar_t* string);

  inline void free(const char* string) { m_pool.free((void*)string); }
  inline void free(const wchar_t* string) { m_pool.free((void*)string); };
  inline void resetFrameMemory() { m_stackAllocator.reset(); }

 private:
  ThreeSizesPool<64, 256> m_pool;
  StackAllocator m_stackAllocator;
};

}  // namespace SirEngine
