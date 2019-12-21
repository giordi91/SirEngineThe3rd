#define CATCH_CONFIG_RUNNER
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/stringPool.h"
#include "catch/catch.hpp"

int main(int argc, char *argv[]) {
  // global setup...
  SirEngine::StringPool stringPool(1024 * 1024 * 20);
  SirEngine::ThreeSizesPool pool(1024 * 1024 * 20);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::globals::PERSISTENT_ALLOCATOR = &pool;
  SirEngine::StackAllocator frameAlloc;
  SirEngine::globals::FRAME_ALLOCATOR = &frameAlloc;
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);

  SirEngine::Log::init();

  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
