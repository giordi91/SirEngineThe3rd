#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/memory/linearBufferManager.h"
#include "catch/catch.hpp"

TEST_CASE("linear buffer manager basic alloc", "[memory]") {
  SirEngine::LinearBufferManage alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(128);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
}

TEST_CASE("linear buffer manager allocs drills 1", "[memory]") {
  SirEngine::LinearBufferManage alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(132);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());

  alloc.free(handle);
  handle = alloc.allocate(64);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 64);
  REQUIRE(range.m_offset == 0); // offset should be zero, reused alloc
}

TEST_CASE("linear buffer manager allocs drills 2", "[memory]") {
  SirEngine::LinearBufferManage alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(72);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 72);
  REQUIRE(range.m_offset == 0); // first alloc

  alloc.free(handle);
  auto handle2 = alloc.allocate(256);
  auto range2 = alloc.getBufferRange(handle2);
  REQUIRE(handle2.isHandleValid());
  REQUIRE(range2.isValid());
  REQUIRE(range2.m_size == 256);
  REQUIRE(range2.m_offset ==
          72); // offset should be as previous alloc
  REQUIRE(alloc.getAllocationsCount() == 1);
  REQUIRE(alloc.getFreeAllocationsCount() == 1);

  handle = alloc.allocate(64);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 64);
  REQUIRE(range.m_offset == 0 ); // offset should be zero, reuse alloc 
  REQUIRE(alloc.getAllocationsCount() == 2);
  REQUIRE(alloc.getFreeAllocationsCount() == 0);
}
