#include "SirEngine/scripting/scriptingContext.h"

#include <cassert>

#include "SirEngine/log.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"

extern "C" {
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>
}

namespace SirEngine {

int helloFromC(lua_State *L)
{
	SE_CORE_INFO("hello from C");
	return 0;
}

ScriptingContext::~ScriptingContext() {
  assert(m_state != nullptr);
  // finish up with the Lua context
  lua_close(m_state);
}
void printError(lua_State *state) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char *message = lua_tostring(state, -1);
  SE_CORE_ERROR(message);
  lua_pop(state, 1);
}

bool ScriptingContext::init(const bool verbose) {
  // create a new lua context to work with
  m_state = luaL_newstate();

  // open any library we may use
  luaL_openlibs(m_state);

  // read the Lua script off disk and execute it
  if ((luaL_dofile(m_state, "../data/scripts/scriptingCore.lua")) != 0) {

    SE_CORE_ERROR("unable to load scriptingCore.lua");
    printError(m_state);
    return false;
  }

  if(verbose) {
	SE_CORE_INFO("Initializing scripting LUA v1.0.0 based on lua 5.3.5");
  }

  registerCFunction("helloFromC",helloFromC);

  return true;
}

ScriptHandle ScriptingContext::loadScript(const char *path,
                                          const bool execute) {

  const int res = luaL_loadfile(m_state, path);
  if (res != LUA_OK) {
    SE_CORE_ERROR("Could not load script {0}, error is:", path);
    printError(m_state);
  }

  // now we have a script, lets generate a handle for it
  const ScriptHandle handle = {MAGIC_NUMBER_COUNTER++};
  const std::string fileName = getFileName(path);
  const char *fileNameStr = persistentString(fileName.c_str());

  lua_setglobal(m_state, fileNameStr);
  m_nameToScript.insert(fileNameStr, handle);
  m_handleToName.insert(handle.handle, fileNameStr);

  if (execute) {
	lua_getglobal(m_state, fileNameStr);
    const int status = lua_pcall(m_state, 0, 0, 0);
	if(status != LUA_OK) {
		printError(m_state);
    }

  }
  return handle;
}

void ScriptingContext::loadScriptsInFolder(const char *path) {}

void ScriptingContext::registerCFunction(const char* name, lua_CFunction function)
{
  //registering functions 
  lua_register(m_state,name,function);
  m_dynamicallyRegisteredFunctions.insert(name,function);
}
} // namespace SirEngine
