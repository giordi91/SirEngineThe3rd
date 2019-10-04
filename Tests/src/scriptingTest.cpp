#include "SirEngine/log.h"
#include "SirEngine/scripting/scriptingContext.h"
#include "catch/catch.hpp"
#include "platform/windows/graphics/dx12/PSOManager.h"

TEST_CASE("scripting init", "[scripting]") {

  SirEngine::Log::init();
  SirEngine::ScriptingContext ctx;
  bool res = ctx.init();
  SirEngine::Log::free();
  REQUIRE(res == true);
}

TEST_CASE("load script", "[scripting]") {
  SirEngine::StringPool stringPool(1024);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();
  SirEngine::ScriptingContext ctx;
  bool res = ctx.init();
  REQUIRE(res == true);
  SirEngine::ScriptHandle handle = ctx.loadScript("../testData/registerTest1.lua", true);
  REQUIRE(handle.isHandleValid());
  handle = ctx.loadScript("../testData/registerTest2.lua", true);
  REQUIRE(handle.isHandleValid());
  SirEngine::Log::free();
}
