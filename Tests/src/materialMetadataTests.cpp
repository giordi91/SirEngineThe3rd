#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "catch/catch.hpp"

TEST_CASE("metadata parse 1", "[material]") {
  const char* path = "../testData/grassForwardPSO.json";
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);
  SirEngine::MaterialMetadata metadata = SirEngine::extractMetadata(path);
}
