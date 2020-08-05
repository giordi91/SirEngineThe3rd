#include "SirEngine/memory/cpu/randomSizeAllocator.h"
#include "catch/catch.hpp"

TEST_CASE("Random size allocator simple allocation", "[memory]") {

  SirEngine::RandomSizeAllocator alloc;
  alloc.initialize(256);
  SirEngine::RandomSizeAllocationHandle mem = alloc.allocate(16);
  char *ptr = alloc.getPointer(mem);
  REQUIRE(ptr == alloc.getStartPtr());
  REQUIRE(alloc.getUnfragmentedPtr() == (ptr + 16));
}

TEST_CASE("Random size allocator multiple allocations", "[memory]") {

  SirEngine::RandomSizeAllocator alloc;
  alloc.initialize(256);
  SirEngine::RandomSizeAllocationHandle mem1 = alloc.allocate(16);
  memset(alloc.getPointer(mem1), 0, mem1.dataSize);
  REQUIRE(alloc.getFreeBlocksCount() == 0);
  SirEngine::RandomSizeAllocationHandle mem2 = alloc.allocate(24);
  memset(alloc.getPointer(mem2), 0, mem2.dataSize);
  REQUIRE(alloc.getFreeBlocksCount() == 0);
  SirEngine::RandomSizeAllocationHandle mem3 = alloc.allocate(48);
  memset(alloc.getPointer(mem3), 0, mem3.dataSize);
  REQUIRE(alloc.getFreeBlocksCount() == 0);
  SirEngine::RandomSizeAllocationHandle mem4 = alloc.allocate(8);
  memset(alloc.getPointer(mem4), 0, mem4.dataSize);
  REQUIRE(alloc.getFreeBlocksCount() == 0);
  SirEngine::RandomSizeAllocationHandle mem5 = alloc.allocate(16);
  memset(alloc.getPointer(mem5), 0, mem5.dataSize);
  REQUIRE(alloc.getFreeBlocksCount() == 0);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());

  // now we performs a deallocation
  char *mem2ptr = alloc.getPointer(mem2);
  alloc.freeAllocation(mem2);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());
  REQUIRE(alloc.getFreeBlocksCount() == 1);

  // now if we re-allocate we should get back the same as mem2 handle, at least
  // memory wise
  SirEngine::RandomSizeAllocationHandle newMem2 = alloc.allocate(12);
  REQUIRE(alloc.getPointer(newMem2) == mem2ptr);
  REQUIRE(newMem2.allocSize == 24);
  REQUIRE(newMem2.dataSize == 12);

  // do a couple more de-alloc
  char *mem3ptr = alloc.getPointer(mem3);
  alloc.freeAllocation(mem3);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());
  char *mem5ptr = alloc.getPointer(mem5);
  alloc.freeAllocation(mem5);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());
  char *mem4ptr = alloc.getPointer(mem4);
  alloc.freeAllocation(mem4);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());

  REQUIRE(alloc.getFreeBlocksCount() == 3);
  SirEngine::RandomSizeAllocationHandle newMem3 = alloc.allocate(18);
  REQUIRE(alloc.getPointer(newMem3) == mem3ptr);
  REQUIRE(alloc.getFreeBlocksCount() == 2);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());
  REQUIRE(newMem3.allocSize == 48);
  REQUIRE(newMem3.dataSize== 18);

  SirEngine::RandomSizeAllocationHandle newMem5 = alloc.allocate(16);
  REQUIRE(alloc.getPointer(newMem5) == mem5ptr);
  REQUIRE(alloc.getFreeBlocksCount() == 1);
  REQUIRE(alloc.getStartPtr() + (112) == alloc.getUnfragmentedPtr());
  REQUIRE(newMem5.allocSize == 16);
  REQUIRE(newMem5.dataSize== 16);

  // now we have one slot free, but we will ask for a bigger memory alloction
  // than the one we have, original mem4 was 8 bytes, so we should see the frag
  // pointer move and number of free block to not decrease
  SirEngine::RandomSizeAllocationHandle newMem4 = alloc.allocate(12);
  REQUIRE(alloc.getPointer(newMem4) != mem4ptr);
  REQUIRE(alloc.getFreeBlocksCount() == 1);
  REQUIRE(alloc.getStartPtr() + (124) == alloc.getUnfragmentedPtr());
  REQUIRE(newMem4.allocSize == 12);
  REQUIRE(newMem4.dataSize== 12);
}
