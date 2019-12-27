#pragma once
#include "farmhash/farmhash.h"

namespace SirEngine {
inline uint32_t hashUint32(const uint32_t &value) {
  uint32_t x = value;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = (x >> 16) ^ x;
  return x;
};
inline uint32_t hashUint16(const uint16_t &value) {
  uint32_t x = value;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = (x >> 16) ^ x;
  return x;
};

inline uint32_t hashUint64(const uint64_t &value) {
  // https://gist.github.com/badboy/6267743
  uint64_t key = value;
  key = (~key) + (key << 18); // key = (key << 18) - key - 1;
  key = key ^ (key >> 31);
  key = key * 21; // key = (key + (key << 2)) + (key << 4);
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return static_cast<uint32_t>(key);
}

inline uint64_t hashString(const char *value, const uint32_t len) {
  return util::Hash64(value, len);
}
inline uint32_t hashString32(const char *const&value) {
  uint32_t len = static_cast<uint32_t>(strlen(value));
  return util::Hash32(value, len);
}

} // namespace SirEngine