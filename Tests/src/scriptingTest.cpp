#include "SirEngine/log.h"
#include "SirEngine/scripting/scriptingContext.h"
#include "catch/catch.hpp"

TEST_CASE("scripting init", "[scripting]") {


  SirEngine::ScriptingContext ctx;
  bool res = ctx.init();
  REQUIRE(res == true);
}

TEST_CASE("load script", "[scripting]") {

  SirEngine::ScriptingContext ctx;
  bool res = ctx.init();
  REQUIRE(res == true);
  SirEngine::ScriptHandle handle =
      ctx.loadScript("../testData/registerTest1.lua", true);
  REQUIRE(handle.isHandleValid());
  handle = ctx.loadScript("../testData/registerTest2.lua", true);
  REQUIRE(handle.isHandleValid());
}
