#include "SirEngine/scripting/scriptingBindings.h"
#include "SirEngine/log.h"
#include "SirEngine/scripting/scriptingContext.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"

extern "C" {
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>
}

namespace SirEngine {

int rotateMainCameraY(lua_State *L)
{
	//number of arguments
	int n = lua_gettop(L);
	assert(n == 1);
	const auto angleInDegrees = static_cast<float>(lua_tonumber(L,1));
	globals::MAIN_CAMERA->spinCameraWorldYAxis(angleInDegrees);
	return 0;
};

void registerFunction(ScriptingContext *ctx, const bool verbose,
                      const char *name, const lua_CFunction func) {

  ctx->registerCFunction(name, func);
  if (verbose) {
    SE_CORE_INFO("[Scripting] Registered builtin function: rotateMainCameraY");
  }
}

void registerBuildInFunctions(ScriptingContext *ctx, const bool verbose) {
  registerFunction(ctx, verbose, "rotateMainCameraY", rotateMainCameraY);
}
} // namespace SirEngine
