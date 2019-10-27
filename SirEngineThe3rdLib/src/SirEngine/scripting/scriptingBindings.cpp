#include "SirEngine/scripting/scriptingBindings.h"
#include "SirEngine/log.h"
#include "SirEngine/scripting/scriptingContext.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/input.h"

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

//expected int as input and returns bool
int inputButtonDown(lua_State* L)
{
	//number of arguments
	int n = lua_gettop(L);
	assert(n == 1);
	const auto buttonCode= static_cast<int>(lua_tonumber(L,1));
	bool isDown = globals::INPUT->isKeyDown(buttonCode);
	lua_pushboolean(L,isDown);
	return 1;
}

int inputButtonWentDownThisFrame(lua_State* L)
{
	//number of arguments
	int n = lua_gettop(L);
	assert(n == 1);
	const auto buttonCode= static_cast<int>(lua_tonumber(L,1));
	bool isDown = globals::INPUT->isKeyPressedThisFrame(buttonCode);
	lua_pushboolean(L,isDown);
	return 1;

	
}


void registerBuiltInFunctions(ScriptingContext *ctx, const bool verbose) {
  registerFunction(ctx, verbose, "rotateMainCameraY", rotateMainCameraY);
  registerFunction(ctx, verbose,"inputButtonDown",inputButtonDown);
  registerFunction(ctx, verbose,"inputButtonWentDownThisFrame",inputButtonWentDownThisFrame);
}
} // namespace SirEngine
