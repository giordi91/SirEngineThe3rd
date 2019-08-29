#pragma once
#include <assert.h>
#include <stdint.h>
#include <string.h>

namespace SirEngine {

template <typename KEY, typename VALUE, uint32_t (*HASH)(const KEY &)>
class HashMap {
public:
  explicit HashMap(const uint32_t bins) : m_bins(bins) {
    m_keys = new KEY[m_bins];
    m_values = new KEY[m_bins];
    const int count = ((m_bins * BIN_FLAGS_SIZE) / (8 * sizeof(uint32_t))) + 1;
    m_metadata = new uint32_t[count];
    // 85 is 01010101 in binary this means we fill 4 bins with the value of 1,
    // meaning free
    memset(m_metadata, 85, count * sizeof(uint32_t));
  }

  ~HashMap() {
    delete[] m_keys;
    delete[] m_values;
    delete[] m_metadata;
  }
  bool insert(KEY key, VALUE value) {
    const uint32_t computedHash = HASH(key);

    // modding wit the bin count
    uint32_t bin = computedHash % m_bins;
    if (m_keys[bin] == key) {
      // key exists we just override the value
      m_values[bin] = value;
      return true;
    }

    const uint32_t startBin = bin;
    uint32_t meta = getMetadata(bin);
    bool free = canWriteToBin(meta);
    while (!free) {
      ++bin;
      bin = bin % m_bins; // wrap around the bins count
      meta = getMetadata(bin);
      free = canWriteToBin(meta);
      if (bin == startBin) {
        return false;
      }
    }
    m_keys[bin] = key;
    m_values[bin] = value;
    writeToBin(bin, key, value);
    setMetadata(bin, BIN_FLAGS::USED);

    return true;
  }

  [[nodiscard]] bool containsKey(const KEY key) const {

    uint32_t bin = 0;
    bool isKeyFound = getBin(key, bin);
    const uint32_t meta = getMetadata(bin);
    return isKeyFound & (m_keys[bin] == key) // does the key match?
           &
           (meta == static_cast<uint32_t>(
                        BIN_FLAGS::USED)); // is the bin actually used, we dont
                                           // delete anything so if they key is
                                           // deleted key value might still
                                           // match but metadata is set to free
  }

  inline bool get(KEY key, VALUE &value) const {
    uint32_t bin = 0;
    const bool result = getBin(key, bin);
    value = m_values[bin];
    return result;
  }

  inline bool remove(KEY key) {
    uint32_t bin = 0;
    const bool result = getBin(key, bin);
    const uint32_t meta = getMetadata(bin);
    assert(meta == static_cast<uint32_t>(BIN_FLAGS::USED));
    if (result) {
      setMetadata(bin, BIN_FLAGS::DELETED);
      --m_usedBins;
    }
    return result;
  }

  [[nodiscard]] uint32_t getUsedBins() const { return m_usedBins; }

  // deleted functions
  HashMap(const HashMap &) = delete;
  HashMap &operator=(const HashMap &) = delete;

private:
  enum class BIN_FLAGS { NONE = 0, FREE = 1, DELETED = 2, USED = 3 };

  bool getBin(const KEY key, uint32_t &bin) const {
    const uint32_t computedHash = HASH(key);
    bin = computedHash % m_bins;
    const uint32_t startBin = bin;

    bool go = true;
    while (go) {
      const uint32_t meta = getMetadata(bin);
      if ((key == m_keys[bin]) |
          (meta == static_cast<uint32_t>(BIN_FLAGS::FREE))) {
        break;
      }

      ++bin;
      bin = bin % m_bins; // wrap around the bins count
      if (bin == startBin) {
        go = false;
      }
    }
    return true;
  }

  inline bool canWriteToBin(const uint32_t metadata) {
    return (metadata == static_cast<uint32_t>(BIN_FLAGS::FREE)) |
           (metadata == static_cast<uint32_t>(BIN_FLAGS::DELETED));
  }

  inline void writeToBin(uint32_t bin, KEY key, VALUE value) {
    m_keys[bin] = key;
    m_values[bin] = value;
    ++m_usedBins;
  }

  inline void setMetadata(const uint32_t bin, BIN_FLAGS flag) {
    const uint32_t bit = bin * BIN_FLAGS_SIZE;
    const uint32_t bit32 = bit / 32;
    const uint32_t reminder32 = bit % 32;
    const uint32_t flag32 = static_cast<uint32_t>(flag);
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
  // this is the number of bytes required for
  static constexpr uint32_t BIN_FLAGS_SIZE = 2;
  static constexpr uint32_t BIN_FLAGS_MASK = 3; // first two bit sets

  KEY *m_keys;
  VALUE *m_values;
  uint32_t *m_metadata;
  uint32_t m_bins;
  uint32_t m_usedBins = 0;
};
} // namespace SirEngine
