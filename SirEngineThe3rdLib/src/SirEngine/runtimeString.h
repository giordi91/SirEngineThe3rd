#pragma once
#include "SirEngine/globals.h"
#include "SirEngine/memory/stringPool.h"
namespace SirEngine {
inline const char* persistentString(const char* string) {
  return globals::STRING_POOL->allocatePersistent(string);
}
inline const wchar_t* persistentConvertWide(const char* string) {
  return globals::STRING_POOL->convertWide(string);
}

inline const char* frameConcatenation(const char* first, const char* second, const char* joiner="") {
  return globals::STRING_POOL->concatenateFrame(first, second, joiner);
}
inline const char* frameConcatenation(const char* first, const int second, const char* joiner="") {
  char temp[40];
  _itoa(second,temp,10);
  return globals::STRING_POOL->concatenateFrame(first, temp, joiner);
}
inline const char* frameConcatenation(const float first, const float second, const char* joiner="") {
  char firstTemp[40];
  char secondTemp[40];
  sprintf(firstTemp,"%.9g",first);
  sprintf(secondTemp,"%.9g",second);

  return globals::STRING_POOL->concatenateFrame(firstTemp, secondTemp, joiner);
}

inline const wchar_t* frameConvertWide(const char* first) {
  return globals::STRING_POOL->convertFrameWide(first);
}
inline const char* frameConvert(const wchar_t* first) {
  return globals::STRING_POOL->convertFrame(first);
}

inline const char* persistentFileLoad(const char* path, uint32_t& fileSize) {
  return globals::STRING_POOL->loadFilePersistent(path, fileSize);
}
inline const char* frameFileLoad(const char* path, uint32_t& fileSize) {
  return globals::STRING_POOL->loadFileFrame(path, fileSize);
}

inline void stringFree(const char* string) {
  globals::STRING_POOL->free(string);
}

}  // namespace SirEngine