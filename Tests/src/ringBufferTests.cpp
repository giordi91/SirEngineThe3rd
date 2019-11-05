#include "SirEngine/memory/ringBuffer.h"
#include "catch/catch.hpp"
#include <iostream>

TEST_CASE("ring buffer create internal opt", "[memory]") {
  SirEngine::RingBuffer<uint32_t> ring(9);
  REQUIRE(ring.usesSmallSizeOptimization() == true);
}

TEST_CASE("ring buffer create no allocator", "[memory]") {
  SirEngine::RingBuffer<uint32_t> ring(15);
  REQUIRE(ring.usesSmallSizeOptimization() == false);
}
TEST_CASE("ring buffer create with allocator ", "[memory]") {
  SirEngine::ThreeSizesPool pool(300 * sizeof(uint32_t));
  SirEngine::RingBuffer<uint32_t> ring(15,&pool);
  REQUIRE(ring.usesSmallSizeOptimization() == false);
}

TEST_CASE("ring buffer push 1 internal opt", "[memory]") {
  SirEngine::RingBuffer<uint32_t> ring(9);
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
TEST_CASE("ring buffer push 1 no alloc", "[memory]") {
  SirEngine::RingBuffer<uint32_t> ring(20);
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

TEST_CASE("ring buffer push 1 with alloc", "[memory]") {
  SirEngine::ThreeSizesPool pool(300 * sizeof(uint32_t));
  SirEngine::RingBuffer<uint32_t> ring(100,&pool);
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

TEST_CASE("ring buffer small", "[memory]") {
  SirEngine::RingBuffer<uint32_t> ring(3);
  bool res = ring.push(10);
  REQUIRE(res==true);
  res = ring.push(12);
  REQUIRE(res==true);
  res = ring.push(14);
  REQUIRE(res==true);
  res = ring.push(17);
  REQUIRE(res==false);
  REQUIRE(ring.isFull()==true);
}
