#pragma once
#include "SirEngine/memory/threeSizesPool.h"
namespace SirEngine {

class StringPool final {
 public:
  enum class STRING_TYPE { CHAR = 1, WIDECHAR = 2 };

 public:
  explicit StringPool(const uint32_t poolSizeInByte) : m_pool(poolSizeInByte){};
  // deleted copy constructors and assignment operator
  StringPool(const StringPool&) = delete;
  StringPool& operator=(const StringPool&) = delete;

  const char* allocate(const char* string) {
    uint32_t length = strlen(string) + 1;
    auto flags = static_cast<uint8_t>(STRING_TYPE::CHAR);
    void* memory = m_pool.allocate(length, flags);
    memcpy(memory, string, length);
	return reinterpret_cast<char*>(memory);
  }

  const wchar_t* allocate(const wchar_t* string) {
    uint32_t length = wcslen(string) + 1;
    auto flags = static_cast<uint8_t>(STRING_TYPE::WIDECHAR);
    uint32_t actualSize = length * sizeof(wchar_t);
    void* memory = m_pool.allocate(actualSize, flags);
    memcpy(memory, string, actualSize);
	return reinterpret_cast<wchar_t*>(memory);
  }
  void free(const char* string) { m_pool.free((void*)string); }
  void free(const wchar_t* string) { m_pool.free((void*)string); };

 private:
  ThreeSizesPool<64, 256> m_pool;
};

}  // namespace SirEngine
