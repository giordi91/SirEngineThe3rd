#include "SirEngine/scripting/scriptingContext.h"
#include "catch/catch.hpp"

TEST_CASE("scripting init", "[scripting]") {

  SirEngine::ScriptingContext ctx;
  ctx.init();
  // REQUIRE(vec.reservedSize() == 10);
}
