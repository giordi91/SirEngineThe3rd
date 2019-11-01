#include "SirEngine/scripting/scriptingContext.h"

#include <cassert>

#include "SirEngine/log.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/scripting/scriptingBindings.h"

extern "C" {
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>
}

namespace SirEngine {

static const char *CORE_SCRIPT_PATH = "../data/scripts/scriptingCore.lua";
static const char *RUN_ANIM = "../data/scripts/runAnim.lua";

ScriptingContext::~ScriptingContext() { cleanup(); }
void printError(lua_State *state) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char *message = lua_tostring(state, -1);
  SE_CORE_ERROR(message);
  lua_pop(state, 1);
}

bool ScriptingContext::init(const bool verbose) {

  bool res = initContext();
  assert(res);
  if (verbose) {
    SE_CORE_INFO("Initializing scripting LUA v1.0.0 based on lua 5.3.5");
  }

  registerBuiltInFunctions(this, verbose);

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
  const std::string fileName = getFileName(path);
  const char *fileNameStr = persistentString(fileName.c_str());

  lua_setglobal(m_state, fileNameStr);
  int id = m_userLoadedScripts.size();
  const ScriptHandle handle{(MAGIC_NUMBER_COUNTER << 16) | id};
  m_userLoadedScripts.pushBack(
      ScriptData{path, fileNameStr, execute, handle, MAGIC_NUMBER_COUNTER});
  m_nameToScript.insert(fileNameStr, handle);

  MAGIC_NUMBER_COUNTER += 1;

  if (execute) {
    lua_getglobal(m_state, fileNameStr);
    const int status = lua_pcall(m_state, 0, 0, 0);
    if (status != LUA_OK) {
      printError(m_state);
    }
  }
  return handle;
}

void ScriptingContext::loadScriptsInFolder(const char *) {}

void ScriptingContext::registerCFunction(const char *name,
                                         lua_CFunction function) {
  // registering functions
  lua_register(m_state, name, function);
  m_dynamicallyRegisteredFunctions.insert(name, function);
}

void ScriptingContext::reloadContext(const char *offsetPath) {
  cleanup();
  initContext();

  // need to re-register all the functions in the state
  int binCount = m_dynamicallyRegisteredFunctions.binCount();
  for (int i = 0; i < binCount; ++i) {
    if (!m_dynamicallyRegisteredFunctions.isBinUsed(i)) {
      continue;
    }
    const char *name = m_dynamicallyRegisteredFunctions.getKeyAtBin(i);
    lua_CFunction func = m_dynamicallyRegisteredFunctions.getValueAtBin(i);
    lua_register(m_state, name, func);
  }

  // now we need to load all the scripts that were loaded and reload them and
  // make them point to the same handle
  uint32_t count = m_userLoadedScripts.size();
  for (uint32_t i = 0; i < count; ++i) {
    const char *path = m_userLoadedScripts[i].path;
    bool execute = m_userLoadedScripts[i].shouldExecute;
    // load/run the script
    const int res =
        luaL_loadfile(m_state, frameConcatenation(offsetPath, path));
    lua_setglobal(m_state, m_userLoadedScripts[i].fileName);
    if (res != LUA_OK) {
      SE_CORE_ERROR("Could not load script {0}, error is:", path);
      assert(0);
      printError(m_state);
    }

    if (execute) {
      lua_getglobal(m_state, m_userLoadedScripts[i].fileName);
      const int status = lua_pcall(m_state, 0, 0, 0);
      if (status != LUA_OK) {
        assert(0);
        printError(m_state);
      }
    }
  }
}

void ScriptingContext::runScriptSlot(SCRIPT_CALLBACK_SLOT slot) const {
  const ScriptHandle handle = m_callbackHandles[static_cast<uint32_t>(slot)];
  assert(handle.isHandleValid());
  int id = getIndexFromHandle(handle);
  const ScriptData &data = m_internalScripts[id];
  lua_getglobal(m_state, data.fileName);
  const int status = lua_pcall(m_state, 0, 0, 0);
  if (status != LUA_OK) {
    printError(m_state);
    assert(0);
  }
}

void ScriptingContext::executeFromHandle(const ScriptHandle handle) const {
  assert(handle.isHandleValid());
  const int id = getIndexFromHandle(handle);
  const ScriptData &data = m_userLoadedScripts[id];
  lua_getglobal(m_state, data.fileName);
  const int status = lua_pcall(m_state, 0,0,0);
  if (status != LUA_OK) {
    printError(m_state);
    assert(0);
  }
  lua_pop(m_state,1);
}

void ScriptingContext::loadAllInternalScripts()
{
  //making sure there is nothing left in the list in case of a reload
  m_internalScripts.clear();

  m_callbackHandles[static_cast<uint32_t>(SCRIPT_CALLBACK_SLOT::PRE_ANIM)] =
      loadScriptInternal(RUN_ANIM, false);
}

bool ScriptingContext::initContext() {
  // create a new lua context to work with
  m_state = luaL_newstate();

  // open any library we may use
  luaL_openlibs(m_state);

  // read the Lua script off disk and execute it
  if ((luaL_dofile(m_state, CORE_SCRIPT_PATH)) != 0) {
    SE_CORE_ERROR(frameConcatenation("unable to load: ", CORE_SCRIPT_PATH));
    printError(m_state);
    return false;
  }
  loadAllInternalScripts();

  return true;
}

void ScriptingContext::cleanup() {
  assert(m_state != nullptr);
  // finish up with the Lua context
  lua_close(m_state);
}

ScriptHandle ScriptingContext::loadScriptInternal(const char *path,
                                                  const bool execute) {
  const int res = luaL_loadfile(m_state, path);
  if (res != LUA_OK) {
    SE_CORE_ERROR("Could not load script {0}, error is:", path);
    printError(m_state);
  }

  // now we have a script, lets generate a handle for it
  const std::string fileName = getFileName(path);
  const char *fileNameStr = persistentString(fileName.c_str());

  lua_setglobal(m_state, fileNameStr);
  int id = m_internalScripts.size();
  const ScriptHandle handle{(MAGIC_INTERNAL_NUMBER_COUNTER << 16) | id};
  // loaded in a different
  m_internalScripts.pushBack(
      ScriptData{path, fileNameStr, execute, handle, MAGIC_NUMBER_COUNTER});

  MAGIC_INTERNAL_NUMBER_COUNTER += 1;

  if (execute) {
    lua_getglobal(m_state, fileNameStr);
    const int status = lua_pcall(m_state, 0, 0, 0);
    if (status != LUA_OK) {
      printError(m_state);
    }
  }
  return handle;
}
} // namespace SirEngine
