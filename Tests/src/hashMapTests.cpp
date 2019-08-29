#include "SirEngine/memory/hashMap.h"
#include "catch/catch.hpp"
#include <iostream>

static uint32_t hashUint(const uint32_t &value) {
  uint32_t x = value;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = ((x >> 16) ^ x) * 0x119de1f3;
  x = (x >> 16) ^ x;
  return x;
};

TEST_CASE("hashmap insert ", "[memory]") {
  SirEngine::HashMap<uint32_t, uint32_t, hashUint> alloc(200);
  alloc.insert(22, 1024);
  alloc.insert(99, 2013);
  alloc.insert(90238409, 21233);

  uint32_t value;
  REQUIRE(alloc.containsKey(22) == true);
  REQUIRE(alloc.get(22, value) == true);
  REQUIRE(value == 1024);
  REQUIRE(alloc.containsKey(99) == true);
  REQUIRE(alloc.get(99, value) == true);
  REQUIRE(value == 2013);
  REQUIRE(alloc.containsKey(90238409) == true);
  REQUIRE(alloc.get(90238409, value) == true);
  REQUIRE(value == 21233);
  REQUIRE(alloc.getUsedBins() == 3);
}

TEST_CASE("hashmap psudo random insert 1000", "[memory]") {
  SirEngine::HashMap<uint32_t, uint32_t, hashUint> alloc(2000);
  std::vector<uint32_t> keys;
  const int count = 1000;
  keys.reserve(count);
  std::vector<uint32_t> values;
  values.reserve(count);

  uint32_t checkValue = 0;
  for (int i = 0; i < count; ++i) {
    uint32_t k = rand();
    uint32_t v = rand();
    // to avoid duplicates
    while (alloc.containsKey(k)) {
      k = rand();
      v = rand();
    }
    alloc.insert(k, v);
    keys.push_back(k);
    values.push_back(v);
    REQUIRE(alloc.containsKey(k) == true);
    REQUIRE(alloc.get(k, checkValue) == true);
    REQUIRE(checkValue == v);
  }

  for (int i = 0; i < count; ++i) {

    CHECKED_ELSE(alloc.containsKey(keys[i]) == true) {
      std::cout << "failed on index " << i << " key values is: " << keys[i]
                << std::endl;
      FAIL();
    };
    REQUIRE(alloc.get(keys[i], checkValue) == true);
    REQUIRE(checkValue == values[i]);
  }

  REQUIRE(alloc.getUsedBins() == count);
}

TEST_CASE("hashmap psudo random insert 1500", "[memory]") {
  SirEngine::HashMap<uint32_t, uint32_t, hashUint> alloc(2000);
  std::vector<uint32_t> keys;
  const int count = 1500;
  keys.reserve(count);
  std::vector<uint32_t> values;
  values.reserve(count);

  uint32_t checkValue = 0;
  for (int i = 0; i < count; ++i) {
    uint32_t k = rand();
    uint32_t v = rand();
    // to avoid duplicates
    while (alloc.containsKey(k)) {
      k = rand();
      v = rand();
    }
    alloc.insert(k, v);
    keys.push_back(k);
    values.push_back(v);
    REQUIRE(alloc.containsKey(k) == true);
    REQUIRE(alloc.get(k, checkValue) == true);
    REQUIRE(checkValue == v);
  }

  for (int i = 0; i < count; ++i) {

    CHECKED_ELSE(alloc.containsKey(keys[i]) == true) {
      std::cout << "failed on index " << i << " key values is: " << keys[i]
                << std::endl;
      FAIL();
    };
    REQUIRE(alloc.get(keys[i], checkValue) == true);
    REQUIRE(checkValue == values[i]);
  }

  REQUIRE(alloc.getUsedBins() == count);
}

TEST_CASE("hashmap psudo random insert 1900", "[memory]") {
  SirEngine::HashMap<uint32_t, uint32_t, hashUint> alloc(2000);
  std::vector<uint32_t> keys;
  const int count = 1900;
  keys.reserve(count);
  std::vector<uint32_t> values;
  values.reserve(count);

  uint32_t checkValue = 0;
  for (int i = 0; i < count; ++i) {
    uint32_t k = rand();
    uint32_t v = rand();
    while (alloc.containsKey(k)) {
      k = rand();
      v = rand();
    }
    alloc.insert(k, v);
    keys.push_back(k);
    values.push_back(v);
    REQUIRE(alloc.containsKey(k) == true);
    REQUIRE(alloc.get(k, checkValue) == true);
    REQUIRE(checkValue == v);
  }

  for (int i = 0; i < count; ++i) {

    CHECKED_ELSE(alloc.containsKey(keys[i]) == true) {
      std::cout << "failed on index " << i << " key values is: " << keys[i]
                << std::endl;
      FAIL();
    };
    REQUIRE(alloc.get(keys[i], checkValue) == true);
    REQUIRE(checkValue == values[i]);
  }

  REQUIRE(alloc.getUsedBins() == count);
}



TEST_CASE("hashmap remove key", "[memory]") {

  SirEngine::HashMap<uint32_t, uint32_t, hashUint> alloc(200);
  alloc.insert(22, 1024);
  alloc.insert(99, 2013);
  alloc.insert(90238409, 21233);

  REQUIRE(alloc.getUsedBins() == 3);
  REQUIRE(alloc.remove(99)==true);
  REQUIRE(alloc.containsKey(99)==false);
  REQUIRE(alloc.getUsedBins() == 2);
  REQUIRE(alloc.remove(22)==true);
  REQUIRE(alloc.containsKey(22)==false);
  REQUIRE(alloc.getUsedBins() == 1);
  REQUIRE(alloc.remove(90238409)==true);
  REQUIRE(alloc.containsKey(90238409)==false);
  REQUIRE(alloc.getUsedBins() == 0);

}
