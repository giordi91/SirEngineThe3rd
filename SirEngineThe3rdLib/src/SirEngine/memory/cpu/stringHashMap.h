#pragma once
#include <stdint.h>
#include <string.h>

#include "SirEngine/globals.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/cpu/hashMap.h"
#include "stringPool.h"

namespace SirEngine {

template <typename VALUE>
class HashMap<const char *, VALUE, hashString32> {
 public:
  // TODO add use of engine allocator, not only heap allocations
  explicit HashMap(const uint32_t bins) : m_bins(bins) {
    m_keys = new const char *[m_bins];
    m_values = new VALUE[m_bins];
    const int count = ((m_bins * BIN_FLAGS_SIZE) / (8 * sizeof(uint32_t))) + 1;
    m_metadata = new uint32_t[count];
    // 85 is 01010101 in binary this means we fill 4 bins with the value of 1,
    // meaning free
    memset(m_keys, 0, m_bins * sizeof(char *));
    memset(m_metadata, 85, count * sizeof(uint32_t));
  }

  ~HashMap() {
    delete[] m_keys;
    delete[] m_values;
    delete[] m_metadata;
  }
  bool insert(const char *key, VALUE value) {
    const uint32_t computedHash = hashString32(key);

    // modding wit the bin count
    uint32_t bin = computedHash % m_bins;
    uint32_t meta = getMetadata(bin);
    if ((m_keys[bin] != nullptr && strcmp(m_keys[bin], key) == 0) &
        (meta == static_cast<uint32_t>(BIN_FLAGS::USED))) {
      // key exists we just override the value
      m_values[bin] = value;
      return true;
    }

    const uint32_t startBin = bin;
    //this might still fail if you actually store null pointer as a string.
    bool free = canWriteToBin(meta);
    while ((!free) && !(m_keys[bin] != nullptr && strcmp(m_keys[bin], key) == 0)) {
      ++bin;
      bin = bin % m_bins;  // wrap around the bins count
      meta = getMetadata(bin);
      free = canWriteToBin(meta);
      if (bin == startBin) {
        return false;
      }
    }
    const char *newKey = globals::STRING_POOL->allocatePersistent(key);
    writeToBin(bin, newKey, value);
    setMetadata(bin, BIN_FLAGS::USED);

    return true;
  }

  [[nodiscard]] bool containsKey(const char *key) const {
    uint32_t bin = 0;
    const bool isKeyFound = getBin(key, bin);
    const uint32_t meta = getMetadata(bin);
    return isKeyFound &
           (m_keys[bin] != nullptr &&
            strcmp(m_keys[bin], key) == 0)  // does the key match?
           &
           (meta == static_cast<uint32_t>(
                        BIN_FLAGS::USED));  // is the bin actually used, we dont
                                            // delete anything so if they key is
                                            // deleted key value might still
                                            // match but metadata is set to free
  }

  inline bool get(const char *key, VALUE &value) const {
    uint32_t bin = 0;
    const bool result = getBin(key, bin);
    value = m_values[bin];
    return result;
  }

  inline bool remove(const char *key) {
    uint32_t bin = 0;
    const bool result = getBin(key, bin);
    const uint32_t meta = getMetadata(bin);
    assert(meta == static_cast<uint32_t>(BIN_FLAGS::USED));
    if (result) {
      setMetadata(bin, BIN_FLAGS::DELETED);
      globals::STRING_POOL->free(m_keys[bin]);
      m_keys[bin] = nullptr;
      --m_usedBins;
    }
    return result;
  }

  [[nodiscard]] uint32_t getUsedBins() const { return m_usedBins; }
  inline uint32_t binCount() const { return m_bins; }
  inline bool isBinUsed(const uint32_t bin) const {
    assert(bin < m_bins);
    uint32_t meta = getMetadata(bin);
    return meta == static_cast<uint32_t>(BIN_FLAGS::USED);
  }

  const char *getKeyAtBin(const uint32_t bin) const {
    // no check done whether the bin is used or not, up to you kid
    assert(bin < m_bins);
    return m_keys[bin];
  }
  VALUE getValueAtBin(uint32_t bin) {
    // no check done whether the bin is used or not, up to you kid
    assert(bin < m_bins);
    return m_values[bin];
  }

  // deleted functions
  HashMap(const HashMap &) = delete;
  HashMap &operator=(const HashMap &) = delete;

 private:
  enum class BIN_FLAGS { NONE = 0, FREE = 1, DELETED = 2, USED = 3 };

  bool getBin(const char *key, uint32_t &bin) const {
    const uint32_t computedHash = hashString32(key);
    bin = computedHash % m_bins;
    const uint32_t startBin = bin;

    bool go = true;
    bool status = true;
    while (go) {
      const uint32_t meta = getMetadata(bin);
      const bool isKeyTheSame =
          m_keys[bin] != nullptr && strcmp(key, m_keys[bin]) == 0;
      const bool isBinUsed = meta == static_cast<uint32_t>(BIN_FLAGS::USED);
      if (isKeyTheSame & isBinUsed) {
        break;
      }

      ++bin;
      bin = bin % m_bins;  // wrap around the bins count
      if ((bin == startBin) | !isBinUsed) {
        go = false;
        status = false;
      }
    }
    return status;
  }

  inline bool canWriteToBin(const uint32_t metadata) {
    return (metadata == static_cast<uint32_t>(BIN_FLAGS::FREE)) |
           (metadata == static_cast<uint32_t>(BIN_FLAGS::DELETED));
  }

  inline void writeToBin(uint32_t bin, const char *key, VALUE value) {
    m_keys[bin] = key;
    m_values[bin] = value;
    ++m_usedBins;
  }

  inline void setMetadata(const uint32_t bin, BIN_FLAGS flag) {
    const uint32_t bit = bin * BIN_FLAGS_SIZE;
    const uint32_t bit32 = bit / 32;
    const uint32_t reminder32 = bit % 32;
    const auto flag32 = static_cast<uint32_t>(flag);
    // first we want to clear the value
    const uint32_t mask = (~0) & ~(3 << reminder32);
    m_metadata[bit32] &= mask;
    // now set the value
    m_metadata[bit32] |= flag32 << reminder32;
  }

  inline uint32_t getMetadata(const uint32_t bin) const {
    const uint32_t bit = bin * BIN_FLAGS_SIZE;
    const uint32_t bit32 = bit / 32;
    const uint32_t reminder32 = bit % 32;
    const uint32_t mask = 3 << reminder32;

    const uint32_t binMetadata = (m_metadata[bit32] & mask) >> reminder32;
    return binMetadata;
  }

 private:
  // this is the number of bits required for bin
  static constexpr uint32_t BIN_FLAGS_SIZE = 2;
  static constexpr uint32_t BIN_FLAGS_MASK = 3;  // first two bit sets

  const char **m_keys;
  VALUE *m_values;
  uint32_t *m_metadata;
  uint32_t m_bins;
  uint32_t m_usedBins = 0;
};
}  // namespace SirEngine
