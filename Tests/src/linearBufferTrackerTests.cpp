#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/memory/linearBufferManager.h"
#include "catch/catch.hpp"

TEST_CASE("linear buffer manager basic alloc", "[memory]") {
  SirEngine::LinearBufferManager alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(128, 1);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
}

TEST_CASE("linear buffer manager allocs drills 1", "[memory]") {
  SirEngine::LinearBufferManager alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(132, 1);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());

  alloc.free(handle);
  handle = alloc.allocate(64, 1);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 64);
  REQUIRE(range.m_offset == 0); // offset should be zero, reused alloc
}

TEST_CASE("linear buffer manager allocs drills 2", "[memory]") {
  SirEngine::LinearBufferManager alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(72, 1);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 72);
  REQUIRE(range.m_offset == 0); // first alloc

  alloc.free(handle);
  auto handle2 = alloc.allocate(256, 1);
  auto range2 = alloc.getBufferRange(handle2);
  REQUIRE(handle2.isHandleValid());
  REQUIRE(range2.isValid());
  REQUIRE(range2.m_size == 256);
  REQUIRE(range2.m_offset == 72); // offset should be as previous alloc
  REQUIRE(alloc.getAllocationsCount() == 1);
  REQUIRE(alloc.getFreeAllocationsCount() == 1);

  handle = alloc.allocate(64, 1);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 64);
  REQUIRE(range.m_offset == 0); // offset should be zero, reuse alloc
  REQUIRE(alloc.getAllocationsCount() == 2);
  REQUIRE(alloc.getFreeAllocationsCount() == 0);
}
TEST_CASE("linear buffer manager alloc to big 1", "[memory]") {
  SirEngine::LinearBufferManager alloc(100);
  auto handle = alloc.allocate(128, 1);
  REQUIRE(!handle.isHandleValid());
}

TEST_CASE("linear buffer manager allocs too big 2", "[memory]") {
  SirEngine::LinearBufferManager alloc(600);
  auto handle = alloc.allocate(72, 1);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 72);
  REQUIRE(range.m_offset == 0); // first alloc

  alloc.free(handle);
  auto handle2 = alloc.allocate(256, 1);
  auto range2 = alloc.getBufferRange(handle2);
  REQUIRE(handle2.isHandleValid());
  REQUIRE(range2.isValid());
  REQUIRE(range2.m_size == 256);
  REQUIRE(range2.m_offset == 72); // offset should be as previous alloc
  REQUIRE(alloc.getAllocationsCount() == 1);
  REQUIRE(alloc.getFreeAllocationsCount() == 1);

  handle = alloc.allocate(64, 1);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_size == 64);
  REQUIRE(range.m_offset == 0); // offset should be zero, reuse alloc
  REQUIRE(alloc.getAllocationsCount() == 2);
  REQUIRE(alloc.getFreeAllocationsCount() == 0);

  handle = alloc.allocate(600, 1);
  REQUIRE(!handle.isHandleValid());
}

TEST_CASE("linear buffer manager basic alloc aligned", "[memory]") {
  SirEngine::LinearBufferManager alloc(2 * SirEngine::MB_TO_BYTE);
  auto handle = alloc.allocate(61, 1);
  auto range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());

  handle = alloc.allocate(128, 128);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_offset == 128);
  REQUIRE(range.m_size == 128);

  handle = alloc.allocate(17, 1);
  handle = alloc.allocate(256, 64);
  range = alloc.getBufferRange(handle);
  REQUIRE(handle.isHandleValid());
  REQUIRE(range.isValid());
  REQUIRE(range.m_offset == 320);
  REQUIRE(range.m_size == 256);
}
