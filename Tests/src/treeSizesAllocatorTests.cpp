#include "SirEngine/memory/threeSizesPool.h"
#include "catch/catch.hpp"

TEST_CASE("Tree sizes pool basic alloc 1", "[memory]") {
  SirEngine::ThreeSizesPool alloc(2 << 16,64,256);
  void *mem = alloc.allocate(16);
  REQUIRE(mem != nullptr);
  REQUIRE(alloc.getSmallAllocCount() == 1);
}

TEST_CASE("Tree sizes pool basic alloc 2", "[memory]") {
  SirEngine::ThreeSizesPool alloc(2 << 16,64,256);
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
  SirEngine::ThreeSizesPool alloc(2 << 16);
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
  SirEngine::ThreeSizesPool alloc(2 << 16);
  void *mem = alloc.allocate(2);
  uint32_t minAllocSize = alloc.getMinAllocSize();
  REQUIRE(mem != nullptr);

  uint32_t allocSize = alloc.getAllocSize(mem);
  uint32_t rawAllocSize = alloc.getRawAllocSize(mem);
  // -4 due to the fact the header is 4 bytes
  REQUIRE(allocSize == (minAllocSize - 4));
  REQUIRE(rawAllocSize == minAllocSize);
}

TEST_CASE("Tree sizes pool delete alloc 1", "[memory]") {
  SirEngine::ThreeSizesPool alloc(2 << 16);
  void *mem = alloc.allocate(16);
  void *mem2 = alloc.allocate(128);
  void *mem3 = alloc.allocate(300);
  void *mem4 = alloc.allocate(128);
  void *mem5 = alloc.allocate(128);
  void *mem6 = alloc.allocate(128);
  memset(mem, 1, 16);
  memset(mem2, 2, 128);
  memset(mem3, 3, 300);
  memset(mem4, 4, 128);
  memset(mem5, 5, 128);
  memset(mem6, 6, 128);
  REQUIRE(mem != nullptr);
  REQUIRE(mem2 != nullptr);
  REQUIRE(mem3 != nullptr);
  REQUIRE(mem4 != nullptr);
  REQUIRE(mem5 != nullptr);
  REQUIRE(mem6 != nullptr);
  REQUIRE(alloc.getMediumAllocCount() == 4);
  REQUIRE(alloc.getSmallAllocCount() == 1);
  REQUIRE(alloc.getLargeAllocCount() == 1);

  // now lets delete some stuff
  alloc.free(mem4);
  REQUIRE(alloc.getMediumAllocCount() == 3);

  void *mem7 = alloc.allocate(120);
  REQUIRE(alloc.getMediumAllocCount() == 4);
  REQUIRE(mem7 == mem4);
  uint32_t memSizeInBtye = alloc.getAllocSize(mem7);
  // allocation should still be 128 since we track the actual
  // original tile allocation
  REQUIRE(memSizeInBtye == 128);
  memset(mem7, 7, 120);

  auto *bytePtr = reinterpret_cast<unsigned char *>(mem7);
  for (uint32_t i = 0; i < 120; ++i) {
    REQUIRE(bytePtr[i] == 7);
  }

#if SE_DEBUG
  for (int i = 120; i < 128; ++i) {
    REQUIRE(bytePtr[i] == 0xff);
  }
#endif
}

TEST_CASE("Tree sizes pool delete alloc 2", "[memory]") {
  SirEngine::ThreeSizesPool alloc(2 << 16);
  void *mem1 = alloc.allocate(128);
  void *mem2 = alloc.allocate(128);
  void *mem3 = alloc.allocate(128);
  void *mem4 = alloc.allocate(128);
  memset(mem1, 1, 128);
  memset(mem2, 2, 128);
  memset(mem3, 3, 128);
  memset(mem4, 4, 128);
  REQUIRE(mem1 != nullptr);
  REQUIRE(mem2 != nullptr);
  REQUIRE(mem3 != nullptr);
  REQUIRE(mem4 != nullptr);
  REQUIRE(alloc.getMediumAllocCount() == 4);

  // now lets delete some stuff
  alloc.free(mem3);
  REQUIRE(alloc.getMediumAllocCount() == 3);
  alloc.free(mem2);
  REQUIRE(alloc.getMediumAllocCount() == 2);

  void *mem5 = alloc.allocate(120);
  REQUIRE(alloc.getMediumAllocCount() == 3);
  REQUIRE(mem5 == mem2);
  // checking memory is properly written and not overrun
  uint32_t memSizeInBtye = alloc.getAllocSize(mem5);
  memset(mem5, 5, memSizeInBtye);
  auto *bytePtr = reinterpret_cast<unsigned char *>(mem5);
  for (uint32_t i = 0; i < memSizeInBtye; ++i) {
    REQUIRE(bytePtr[i] == 5);
  }

#if SE_DEBUG
  for (uint32_t i = memSizeInBtye; i < 128; ++i) {
    REQUIRE(bytePtr[i] == 0xff);
  }
#endif

  void *mem6 = alloc.allocate(70);
  REQUIRE(alloc.getMediumAllocCount() == 4);
  REQUIRE(mem6 == mem3);
  // checking memory is properly written and not overrun
  memSizeInBtye = alloc.getAllocSize(mem6);
  memset(mem6, 6, memSizeInBtye);
  bytePtr = reinterpret_cast<unsigned char *>(mem6);
  for (uint32_t i = 0; i < memSizeInBtye; ++i) {
    REQUIRE(bytePtr[i] == 6);
  }

#if SE_DEBUG
  for (int i = memSizeInBtye; i < 128; ++i) {
    REQUIRE(bytePtr[i] == 0xff);
  }
#endif

  alloc.allocate(200);
  REQUIRE(alloc.getMediumAllocCount() == 5);
}
