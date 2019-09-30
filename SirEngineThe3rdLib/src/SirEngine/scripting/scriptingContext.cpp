#include "SirEngine/scripting/scriptingContext.h"

#include <cassert>

#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

extern "C" 
{
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>
}



namespace SirEngine {
ScriptingContext::~ScriptingContext() {
  assert(m_state != nullptr);
  // finish up with the Lua context
  lua_close(m_state);
}
void print_error(lua_State *state) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char *message = lua_tostring(state, -1);
  SE_CORE_ERROR(message);
  lua_pop(state, 1);
}

void ScriptingContext::init() {
  // create a new lua context to work with
  m_state = luaL_newstate();

  // open any library we may use
  luaL_openlibs(m_state);

  // read the Lua script off disk and execute it
   if ((luaL_dofile(m_state, "../data/scripts/test.lua")) != 0) {

    // handle any errors
    std::cout << "unable to load test.lua" << std::endl;
    return;
    // return 1;
  }

  //// put the value of X at the top of the stack
  // lua_getglobal(m_state, "x");

  //// interpret the value at the top of the stack
  //// as an integer and put it in the variable "val"
  // int val = (int)lua_tointeger(m_state, -1);

  //// pop the value of X off the stack
  // lua_pop(m_state, 1);

  // write the value out
  // std::cout << "Value of X: " << val << std::endl;
}

void ScriptingContext::loadScript(const char *path) {

  const int res = luaL_loadfile(m_state, path);
  if (res != LUA_OK) {
    SE_CORE_ERROR("Could not load script {0}, error is:", path);
    print_error(m_state);
  }

  // now we have a script, lets generate a handle for it
}

void ScriptingContext::loadScriptsInFolder(const char *path) {}
} // namespace SirEngine
