#include "SirEngine/memory/stringPool.h"
#include <cstdio>
#include <cstdlib>

namespace SirEngine {

const char* StringPool::allocatePersistent(const char* string) {
  const auto length = static_cast<uint32_t>(strlen(string) + 1);
  const auto flags = static_cast<uint8_t>(STRING_TYPE::CHAR);
  void* memory = m_pool.allocate(length, flags);
  memcpy(memory, string, length);
  return reinterpret_cast<char*>(memory);
}

const wchar_t* StringPool::allocatePersistent(const wchar_t* string) {
  const uint64_t length = wcslen(string) + 1;
  const auto flags = static_cast<uint8_t>(STRING_TYPE::WCHAR);
  const auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
  void* memory = m_pool.allocate(actualSize, flags);
  memcpy(memory, string, actualSize);
  return reinterpret_cast<wchar_t*>(memory);
}

const char* StringPool::allocateFrame(const char* string) {
  const auto length = static_cast<uint32_t>(strlen(string) + 1);
  void* memory = m_stackAllocator.allocate(length);
  memcpy(memory, string, length);
  return reinterpret_cast<const char*>(memory);
}

const wchar_t* StringPool::allocateFrame(const wchar_t* string) {
  const uint64_t length = wcslen(string) + 1;
  const auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
  void* memory = m_stackAllocator.allocate(actualSize);
  memcpy(memory, string, actualSize);
  return reinterpret_cast<wchar_t*>(memory);
}
inline int isFlagSet(const uint8_t flags,
                     const STRING_MANIPULATION_FLAGS flagToCheck) {
  return (flags & flagToCheck) > 0 ? 1 : 0;
}

const char* StringPool::loadFilePersistent(const char* path) {
  FILE* fp = nullptr;

  const errno_t error = fopen_s(&fp, path, "rb");
  assert((error == 0) && "could not open file");

  fseek(fp, 0L, SEEK_END);
  const long fileSize = ftell(fp);
  rewind(fp);

  // allocating memory
  char* buffer = reinterpret_cast<char*>(m_pool.allocate(fileSize + 1));

  const uint64_t count = fread(buffer, fileSize, 1, fp);
  assert(count == 1 && "error reading actual memory from file");
  //need tos et the final value
  buffer[fileSize] = '\0';

  fclose(fp);
  return buffer;
}

const char* StringPool::concatenatePersistent(const char* first,
                                              const char* second,
                                              const char* joiner,
                                              const uint8_t flags) {
  const int firstInPool = m_pool.allocationInPool(first);
  const int secondInPool = m_pool.allocationInPool(second);
  const int joinerInPool =
      joiner != nullptr ? m_pool.allocationInPool(joiner) : false;

  // this length are without the extra null terminator
  const auto firstLen = static_cast<uint32_t>(strlen(first));
  const auto secondLen = static_cast<uint32_t>(strlen(second));
  const uint32_t joinerLen =
      joiner != nullptr ? static_cast<uint32_t>(strlen(joiner)) : 0u;

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

const wchar_t* StringPool::concatenatePersistentWide(const wchar_t* first,
                                                     const wchar_t* second,
                                                     const wchar_t* joiner,
                                                     const uint8_t flags) {
  const int firstInPool = m_pool.allocationInPool(first);
  const int secondInPool = m_pool.allocationInPool(second);
  const int joinerInPool =
      joiner != nullptr ? m_pool.allocationInPool(joiner) : false;

  // this length are without the extra null terminator
  const auto firstLen = static_cast<uint32_t>(wcslen(first));
  const auto secondLen = static_cast<uint32_t>(wcslen(second));
  const uint32_t joinerLen =
      joiner != nullptr ? static_cast<uint32_t>(wcslen(joiner)) : 0;

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

const char* StringPool::concatenateFrame(const char* first, const char* second,
                                         const char* joiner) {
  // this length are without the extra null terminator
  const auto firstLen = static_cast<uint32_t>(strlen(first));
  const auto secondLen = static_cast<uint32_t>(strlen(second));
  const uint32_t joinerLen =
      joiner != nullptr ? static_cast<uint32_t>(strlen(joiner)) : 0u;

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
  return newChar;
}

const wchar_t* StringPool::concatenateFrameWide(const wchar_t* first,
                                                const wchar_t* second,
                                                const wchar_t* joiner) {
  // this length are without the extra null terminator
  const auto firstLen = static_cast<uint32_t>(wcslen(first));
  const auto secondLen = static_cast<uint32_t>(wcslen(second));
  const uint32_t joinerLen =
      joiner != nullptr ? static_cast<uint32_t>(wcslen(joiner)) : 0;

  // plus one for null terminator
  const uint32_t totalLen =
      (firstLen + secondLen + joinerLen + 1) * sizeof(wchar_t);

  // make the allocation
  auto* newChar =
      reinterpret_cast<wchar_t*>(m_stackAllocator.allocate(totalLen));
  // do the memcpy
  memcpy(newChar, first, firstLen * sizeof(wchar_t));
  if (joinerLen != 0) {
    memcpy(newChar + firstLen, joiner, joinerLen * sizeof(wchar_t));
  }
  // here we copy an extra byte for the termination string
  memcpy(newChar + firstLen + joinerLen, second,
         (secondLen + 1) * sizeof(wchar_t));

  return newChar;
}

const char* StringPool::convert(const wchar_t* string, const uint8_t flags) {
  const int inPool = m_pool.allocationInPool(string);

  // this length are without the extra null terminator
  const auto len = static_cast<uint32_t>(wcslen(string));

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

const char* StringPool::convertFrame(const wchar_t* string) {
  // this length are without the extra null terminator
  const auto len = static_cast<uint32_t>(wcslen(string));
  auto* newChar = reinterpret_cast<char*>(m_stackAllocator.allocate(len + 1));

  // do the conversion
  size_t converted;
  wcstombs_s(&converted, newChar, len + 1, string, len * 2);

  return newChar;
}

const wchar_t* StringPool::convertWide(const char* string,
                                       const uint8_t flags) {
  // this length are without the extra null terminator
  const auto len = static_cast<uint32_t>(strlen(string));

  // plus one for null terminator
  const auto allocFlags = static_cast<uint8_t>(STRING_TYPE::WCHAR);

  // make the allocation
  auto* newChar = reinterpret_cast<wchar_t*>(
      m_pool.allocate(sizeof(wchar_t) * (len + 1), allocFlags));
  // do the conversion
  size_t converted;
  mbstowcs_s(&converted, newChar, (len + 1 * 2), string, len);

  // now we have some clean up to do based on flags
  const int inPool = m_pool.allocationInPool(string);
  const int firstSet = isFlagSet(flags, FREE_FIRST_AFTER_OPERATION);
  const int shouldFreeFirst = firstSet & inPool;
  if (shouldFreeFirst) {
    m_pool.free((void*)string);
  }
  return newChar;
}

const wchar_t* StringPool::convertFrameWide(const char* string) {
  // this length are without the extra null terminator
  const auto len = static_cast<uint32_t>(strlen(string));

  // make the allocation
  auto* newChar = reinterpret_cast<wchar_t*>(
      m_stackAllocator.allocate(sizeof(wchar_t) * (len + 1)));
  // do the conversion
  size_t converted;
  mbstowcs_s(&converted, newChar, (len + 1 * 2), string, len);
  return newChar;
}
}  // namespace SirEngine
