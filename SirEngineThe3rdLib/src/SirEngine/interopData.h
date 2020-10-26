#pragma once

#include "SirEngine/memory/cpu/stringHashMap.h"

namespace SirEngine {

class InteropData {
 public:
  InteropData() : m_data(RESERVE_SIZE){};
  void registerData(const char* key, void* data) {
    //assert(!m_data.containsKey(key));
    m_data.insert(key, data);
  }

  //here we don't do any assertion, it is valid to check if data exists or not
  void* getData(const char* key) const {
    void* toReturn = nullptr;
    m_data.get(key, toReturn);
    return toReturn;
  }

  //mostly here for symmetry with rest of the manager
  void initialize(){}

private:
  HashMap<const char*, void*, hashString32> m_data;
  static constexpr uint32_t RESERVE_SIZE = 200;
};

}  // namespace SirEngine