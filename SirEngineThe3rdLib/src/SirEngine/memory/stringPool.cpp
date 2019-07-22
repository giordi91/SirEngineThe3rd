#include "SirEngine/memory/stringPool.h"
#include <cstdlib>

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
                                          const char* joiner,
                                          const uint8_t flags) {
  const int firstInPool = m_pool.allocationInPool(first);
  const int secondInPool = m_pool.allocationInPool(second);
  const int joinerInPool =
      joiner != nullptr ? m_pool.allocationInPool(joiner) : false;

  // this length are without the extra null terminator
  const uint32_t firstLen = getStaticLength(first, firstInPool);
  const uint32_t secondLen = getStaticLength(second, secondInPool);
  const uint32_t joinerLen =
      joiner != nullptr ? getStaticLength(joiner, joinerInPool) : 0;

  // plus one for null terminator
  const auto allocFlags = static_cast<uint8_t>(STRING_TYPE::CHAR);
  const uint32_t totalLen = firstLen + secondLen + joinerLen + 1;

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
  const int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  const int shouldFreeFirst = firstSet & firstInPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)first);
  }
  const int secondSet = isFlagSet(flags, FREE_SECOND_AFTER_OPERATION);
  const int shouldFreeSecond = secondSet & secondInPool;
  if (shouldFreeSecond) {
    m_pool.free((void*)second);
  }
  const int joinerSet = isFlagSet(flags, FREE_JOINER_AFTER_OPERATION);
  const int shouldFreeJoiner = joinerSet & joinerInPool;
  if (shouldFreeJoiner) {
    m_pool.free((void*)joiner);
  }

  return newChar;
}

const wchar_t* StringPool::concatenateStaticWide(const wchar_t* first,
                                                 const wchar_t* second,
                                                 const wchar_t* joiner,
                                                 const uint8_t flags) {
  const int firstInPool = m_pool.allocationInPool(first);
  const int secondInPool = m_pool.allocationInPool(second);
  const int joinerInPool =
      joiner != nullptr ? m_pool.allocationInPool(joiner) : false;

  // this length are without the extra null terminator
  const uint32_t firstLen = getStaticLength(first, firstInPool);
  const uint32_t secondLen = getStaticLength(second, secondInPool);
  const uint32_t joinerLen =
      joiner != nullptr ? getStaticLength(joiner, joinerInPool) : 0;

  // plus one for null terminator
  const auto allocFlags = static_cast<uint8_t>(STRING_TYPE::CHAR);
  const uint32_t totalLen =
      (firstLen + secondLen + joinerLen + 1) * sizeof(wchar_t);

  // make the allocation
  auto* newChar =
      reinterpret_cast<wchar_t*>(m_pool.allocate(totalLen, allocFlags));
  // do the memcpy
  memcpy(newChar, first, firstLen * sizeof(wchar_t));
  if (joinerLen != 0) {
    memcpy(newChar + firstLen, joiner, joinerLen * sizeof(wchar_t));
  }
  // here we copy an extra byte for the termination string
  memcpy(newChar + firstLen + joinerLen, second,
         (secondLen + 1) * sizeof(wchar_t));

  // now we have some clean up to do based on flags
  const int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  const int shouldFreeFirst = firstSet & firstInPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)first);
  }
  const int secondSet = isFlagSet(flags, FREE_SECOND_AFTER_OPERATION);
  const int shouldFreeSecond = secondSet & secondInPool;
  if (shouldFreeSecond) {
    m_pool.free((void*)second);
  }
  const int joinerSet = isFlagSet(flags, FREE_JOINER_AFTER_OPERATION);
  const int shouldFreeJoiner = joinerSet & joinerInPool;
  if (shouldFreeJoiner) {
    m_pool.free((void*)joiner);
  }

  return newChar;
}

const char* StringPool::convert(const wchar_t* string, uint8_t flags) {
  const int inPool = m_pool.allocationInPool(string);

  // this length are without the extra null terminator
  const uint32_t len = getStaticLength(string, inPool);

  // plus one for null terminator
  const auto allocFlags = static_cast<uint8_t>(STRING_TYPE::CHAR);

  // make the allocation
  auto* newChar = reinterpret_cast<char*>(m_pool.allocate(len + 1, allocFlags));
  // do the conversion
  size_t converted;
  wcstombs_s(&converted, newChar, len + 1, string, len * 2);

  // now we have some clean up to do based on flags
  const int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  const int shouldFreeFirst = firstSet & inPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)string);
  }
  return newChar;
}

const wchar_t* StringPool::convertWide(const char* string, uint8_t flags) {
  const int inPool = m_pool.allocationInPool(string);

  // this length are without the extra null terminator
  const uint32_t len = getStaticLength(string, inPool);

  // plus one for null terminator
  const auto allocFlags = static_cast<uint8_t>(STRING_TYPE::WIDECHAR);

  // make the allocation
  auto* newChar =
      reinterpret_cast<wchar_t*>(m_pool.allocate(len + 1, allocFlags));
  // do the conversion
  size_t converted;
  mbstowcs_s(&converted, newChar, (len + 1 * 2), string, len);

  // now we have some clean up to do based on flags
  const int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  const int shouldFreeFirst = firstSet & inPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)string);
  }
  return newChar;
}

uint32_t StringPool::getStaticLength(const char* string,
                                     const bool isInPool) const {
  if (isInPool) {
    // we are subtracing one because the pool does not know is a string,
    // so returns the allocation size which is with the null terminator
    const int minAllocSize = ThreeSizesPool<64, 256>::getMinAllocSize();
    const int allocSize = m_pool.getAllocSize((void*)string);
    const int headerSize = 4;
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
uint32_t StringPool::getStaticLength(const wchar_t* string,
                                     const bool isInPool) const {
  if (isInPool) {
    // we are subtracing one because the pool does not know is a string,
    // so returns the allocation size which is with the null terminator
    const int minAllocSize = ThreeSizesPool<64, 256>::getMinAllocSize();
    const int allocSize = m_pool.getAllocSize((void*)string);
    const int headerSize = 4;
    if ((minAllocSize - headerSize) == allocSize) {
      // if this is the case probably the string was shorter then then the
      // minimum allocation size we must use regular string len
      return static_cast<uint32_t>(wcslen(string));
    } else {
      return (allocSize / 2) - 1;
    }
  } else {
    return static_cast<uint32_t>(wcslen(string));
  }
}
}  // namespace SirEngine
