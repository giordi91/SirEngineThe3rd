#include "SirEngine/memory/threeSizesPool.h"
#include "catch/catch.hpp"

TEST_CASE("Tree sizes pool basic alloc 1", "[memory]") {
  SirEngine::ThreeSizesPool<64, 256> alloc(2 << 16);
  void *mem = alloc.allocate(16);
  REQUIRE(mem != nullptr);
  REQUIRE(alloc.getSmallAllocCount() == 1);
}

TEST_CASE("Tree sizes pool basic alloc 2", "[memory]") {
  SirEngine::ThreeSizesPool<64, 256> alloc(2 << 16);
  void *mem = alloc.allocate(16);
  void *mem2 = alloc.allocate(128);
  void *mem3 = alloc.allocate(300);
  REQUIRE(mem != nullptr);
  REQUIRE(mem2 != nullptr);
  REQUIRE(mem3 != nullptr);
  REQUIRE(alloc.getSmallAllocCount() == 1);
  REQUIRE(alloc.getMediumAllocCount() == 1);
  REQUIRE(alloc.getLargeAllocCount() == 1);
}

TEST_CASE("Tree sizes pool getAlloc size", "[memory]") {
  SirEngine::ThreeSizesPool<64, 256> alloc(2 << 16);
  void *mem = alloc.allocate(16);
  void *mem2 = alloc.allocate(128);
  void *mem3 = alloc.allocate(300);
  REQUIRE(mem != nullptr);
  REQUIRE(mem2 != nullptr);
  REQUIRE(mem3 != nullptr);

  uint32_t allocSize1 = alloc.getAllocSize(mem);
  uint32_t allocSize2 = alloc.getAllocSize(mem2);
  uint32_t allocSize3 = alloc.getAllocSize(mem3);

  REQUIRE(allocSize1 == 16);
  REQUIRE(allocSize2 == 128);
  REQUIRE(allocSize3 == 300);
}

TEST_CASE("Tree sizes pool min alloc size", "[memory]") {
  SirEngine::ThreeSizesPool<64, 256> alloc(2 << 16);
  void *mem = alloc.allocate(2);
  uint32_t minAllocSize = SirEngine::ThreeSizesPool<64, 256>::getMinAllocSize();
  REQUIRE(mem != nullptr);

  uint32_t allocSize = alloc.getAllocSize(mem);
  uint32_t rawAllocSize = alloc.getRawAllocSize(mem);
  // -4 due to the fact the header is 4 bytes
  REQUIRE(allocSize == (minAllocSize -4));
  REQUIRE(rawAllocSize== minAllocSize);
}

TEST_CASE("Tree sizes pool delete alloc 1", "[memory]") {
  SirEngine::ThreeSizesPool<64, 256> alloc(2 << 16);
  void *mem = alloc.allocate(16);
  void *mem2 = alloc.allocate(128);
  void *mem3 = alloc.allocate(300);
  void *mem4 = alloc.allocate(128);
  void *mem5 = alloc.allocate(128);
  void *mem6 = alloc.allocate(128);
  REQUIRE(mem != nullptr);
  REQUIRE(mem2 != nullptr);
  REQUIRE(mem3 != nullptr);
  REQUIRE(mem4 != nullptr);
  REQUIRE(mem5 != nullptr);
  REQUIRE(mem6 != nullptr);

  //now lets delete some stuff

}

