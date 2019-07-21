#include "SirEngine/memory/stringPool.h"

namespace SirEngine {

const char* StringPool::allocateStatic(const char* string) {
  auto length = static_cast<uint32_t>(strlen(string) + 1);
  auto flags = static_cast<uint8_t>(STRING_TYPE::CHAR);
  void* memory = m_pool.allocate(length, flags);
  memcpy(memory, string, length);
  return reinterpret_cast<char*>(memory);
}

const wchar_t* StringPool::allocateStatic(const wchar_t* string) {
  uint64_t length = wcslen(string) + 1;
  auto flags = static_cast<uint8_t>(STRING_TYPE::WIDECHAR);
  auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
  void* memory = m_pool.allocate(actualSize, flags);
  memcpy(memory, string, actualSize);
  return reinterpret_cast<wchar_t*>(memory);
}

const char* StringPool::allocateFrame(const char* string) {
  auto length = static_cast<uint32_t>(strlen(string) + 1);
  void* memory = m_stackAllocator.allocate(length);
  memcpy(memory, string, length);
  return reinterpret_cast<const char*>(memory);
}

const wchar_t* StringPool::allocateFrame(const wchar_t* string) {
  uint64_t length = wcslen(string) + 1;
  auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
  void* memory = m_stackAllocator.allocate(actualSize);
  memcpy(memory, string, actualSize);
  return reinterpret_cast<wchar_t*>(memory);
}
inline int isFlagSet(const uint8_t flags,
                     const STRING_MANIPULATION_FLAGS flagToCheck) {
  return (flags & flagToCheck) > 0 ? 1 : 0;
}

const char* StringPool::concatenateStatic(const char* first, const char* second,
                                          const char* joiner, uint8_t flags) {
  int firstInPool = m_pool.allocationInPool(first);
  int secondInPool = m_pool.allocationInPool(second);
  int joinerInPool =
      joiner != nullptr ? m_pool.allocationInPool(joiner) : false;

  // this length are without the extra null terminator
  uint32_t firstLen = getStaticLength(first, firstInPool);
  uint32_t secondLen = getStaticLength(second, secondInPool);
  uint32_t joinerLen =
      joiner != nullptr ? getStaticLength(joiner, joinerInPool) : 0;

  // plus one for null terminator
  auto allocFlags = static_cast<uint8_t>(STRING_TYPE::CHAR);
  uint32_t totalLen = firstLen + secondLen + joinerLen + 1;

  // make the allocation
  char* newChar =
      reinterpret_cast<char*>(m_pool.allocate(totalLen, allocFlags));
  // do the memcpy
  memcpy(newChar, first, firstLen);
  if (joinerLen != 0) {
    memcpy(newChar + firstLen, joiner, joinerLen);
  }
  // here we copy an extra byte for the termination string
  memcpy(newChar + firstLen + joinerLen, second, secondLen + 1);

  // now we have some clean up to do based on flags
  int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  int shouldFreeFirst = firstSet & firstInPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)first);
  }
  int secondSet = isFlagSet(flags, FREE_SECOND_AFTER_OPERATION);
  int shouldFreeSecond = secondSet & secondInPool;
  if (shouldFreeSecond) {
    m_pool.free((void*)second);
  }
  int joinerSet = isFlagSet(flags, FREE_JOINER_AFTER_OPERATION);
  int shouldFreeJoiner = joinerSet & joinerInPool;
  if (shouldFreeJoiner) {
    m_pool.free((void*)joiner);
  }

  return newChar;
}

uint32_t StringPool::getStaticLength(const char* string, bool isInPool) const {
  if (isInPool) {
    // we are subtracing one because the pool does not know is a string,
    // so returns the allocation size which is with the null terminator
    int minAllocSize = ThreeSizesPool<64, 256>::getMinAllocSize();
    int allocSize = m_pool.getAllocSize((void*)string);
    int headerSize = 4;
    if ((minAllocSize - headerSize) == allocSize) {
      // if this is the case probably the string was shorter then then the
      // minimum allocation size we must use regular string len
      return static_cast<uint32_t>(strlen(string));
    } else {
      return allocSize - 1;
    }
  } else {
    return static_cast<uint32_t>(strlen(string));
  }
}
}  // namespace SirEngine
