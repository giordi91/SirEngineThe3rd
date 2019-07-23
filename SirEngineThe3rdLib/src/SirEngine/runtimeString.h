#pragma once
#include "SirEngine/globals.h"
#include "SirEngine/memory/stringPool.h"
namespace SirEngine {
inline const char* persistentString(const char* string) {
  return globals::STRING_POOL->allocatePersistent(string);
}

inline const char* frameConcatenation(const char* first, const char* second) {
  return globals::STRING_POOL->concatenateFrame(first, second, "");
}

inline const wchar_t* frameConvertWide(const char* first) {
  return globals::STRING_POOL->convertFrameWide(first);
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