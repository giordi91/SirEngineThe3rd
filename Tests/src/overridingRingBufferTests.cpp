#include <iostream>

#include "SirEngine/memory/cpu/overridingRingBuffer.h"
#include "catch/catch.hpp"

static uint32_t CALLBACK1_SENTINEL = 0;
static uint32_t CALLBACK2_SENTINEL = 0;
void callback1(uint32_t& ) { CALLBACK1_SENTINEL = 1; }
void callback2(uint32_t& ) { CALLBACK2_SENTINEL += 1; }

TEST_CASE("overriding ring buffer destructor no callback", "[memory]") {
  SirEngine::OverridingRingBuffer<uint32_t> ring(9);
}

TEST_CASE("overriding ring buffer destructor callback", "[memory]") {
  {
    CALLBACK1_SENTINEL = 0;
    SirEngine::OverridingRingBuffer<uint32_t> ring(99);
    ring.registerDestroyCallback(callback1);
  }
  REQUIRE(CALLBACK1_SENTINEL == 0);
}

TEST_CASE("overriding ring buffer create with allocator ", "[memory]") {
  {
    CALLBACK1_SENTINEL = 0;
    SirEngine::ThreeSizesPool pool(300 * sizeof(uint32_t));
    SirEngine::OverridingRingBuffer<uint32_t> ring(15, &pool);
    ring.registerDestroyCallback(callback1);
    REQUIRE(CALLBACK1_SENTINEL == 0);
  }
}

TEST_CASE("overriding ring buffer push 1 ", "[memory]") {
  SirEngine::OverridingRingBuffer<uint32_t> ring(9);
  ring.push(10);
  ring.push(12);
  ring.push(14);
  ring.push(17);
  REQUIRE(ring.usedElementCount() == 4);

  REQUIRE(ring.front() == 10);
  REQUIRE(ring.front() == 10);
  REQUIRE(ring.front() == 10);

  uint32_t v = ring.pop();
  REQUIRE(v == 10);
  REQUIRE(ring.front() == 12);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 3);
  v = ring.pop();
  REQUIRE(v == 12);
  REQUIRE(ring.front() == 14);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 2);
  v = ring.pop();
  REQUIRE(v == 14);
  REQUIRE(ring.front() == 17);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 1);
  v = ring.pop();
  REQUIRE(v == 17);
  REQUIRE(ring.isEmpty() == true);
  REQUIRE(ring.usedElementCount() == 0);
}
TEST_CASE("overriding ring buffer push 1 with alloc", "[memory]") {
  SirEngine::ThreeSizesPool pool(300 * sizeof(uint32_t));
  SirEngine::OverridingRingBuffer<uint32_t> ring(100, &pool);
  ring.push(10);
  ring.push(12);
  ring.push(14);
  ring.push(17);
  REQUIRE(ring.usedElementCount() == 4);

  REQUIRE(ring.front() == 10);
  REQUIRE(ring.front() == 10);
  REQUIRE(ring.front() == 10);

  uint32_t v = ring.pop();
  REQUIRE(v == 10);
  REQUIRE(ring.front() == 12);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 3);
  v = ring.pop();
  REQUIRE(v == 12);
  REQUIRE(ring.front() == 14);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 2);
  v = ring.pop();
  REQUIRE(v == 14);
  REQUIRE(ring.front() == 17);
  REQUIRE(ring.isEmpty() == false);
  REQUIRE(ring.usedElementCount() == 1);
  v = ring.pop();
  REQUIRE(v == 17);
  REQUIRE(ring.isEmpty() == true);
  REQUIRE(ring.usedElementCount() == 0);
}

TEST_CASE("overriding ring buffer small", "[memory]") {
  SirEngine::OverridingRingBuffer<uint32_t> ring(3);
  bool res = ring.push(10);
  REQUIRE(res == true);
  res = ring.push(12);
  REQUIRE(res == true);
  res = ring.push(14);
  REQUIRE(res == true);
  REQUIRE(ring.front() == 10);
  REQUIRE(ring.back() == 14);
  res = ring.push(17);
  REQUIRE(res == true);
  REQUIRE(ring.front() == 12);
  REQUIRE(ring.back() == 17);
  REQUIRE(ring.isFull() == true);
}

TEST_CASE("overriding ring buffer iteration", "[memory]") {
  SirEngine::OverridingRingBuffer<uint32_t> ring(16);
  for (int i = 0; i < 20; ++i) {
    bool res = ring.push(i);
    REQUIRE(res);
  }
  uint32_t start = ring.getFirstElementIndex();
  uint32_t end = ring.getLastElementIndex();
  uint32_t size = ring.getSize();
  uint32_t count = ring.usedElementCount();
  const uint32_t* data = ring.getData();

  REQUIRE(start == 4);
  REQUIRE(end == 3);
  for (uint32_t i = 0; i < count; ++i) {
    uint32_t idx = (start + i) % size;
    REQUIRE(data[idx] == i + 4);
  }
}

TEST_CASE("overriding ring buffer destructor callback 2", "[memory]") {
  CALLBACK2_SENTINEL = 0;
  SirEngine::OverridingRingBuffer<uint32_t> ring(20);
  ring.registerDestroyCallback(callback2);

  for (int i = 0; i < 40; ++i) {
    bool res = ring.push(i);
    REQUIRE(res);
  }
  REQUIRE(CALLBACK2_SENTINEL == 20);
}
