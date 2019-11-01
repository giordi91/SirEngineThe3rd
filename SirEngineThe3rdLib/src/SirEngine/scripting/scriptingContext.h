#pragma once
#include "SirEngine/core.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/stringHashMap.h"
#include <cstdint>

// forward declare
struct lua_State;

// c function that can be registerd in lua
typedef int (*lua_CFunction)(lua_State *L);
namespace SirEngine {

enum class SCRIPT_CALLBACK_SLOT {
  PRE_ANIM = 0,
  COUNT,
};
// This should be a private struct, forced to be public due to dll interface
struct ScriptData {
  const char *path;
  const char *fileName;
  bool shouldExecute;
  ScriptHandle handle;
  uint32_t magicNumber;
};

template class SIR_ENGINE_API HashMap<const char *, ScriptHandle, hashString32>;
template class SIR_ENGINE_API HashMap<uint32_t, const char *, hashUint32>;
template class SIR_ENGINE_API
    HashMap<const char *, lua_CFunction, hashString32>;
template class SIR_ENGINE_API ResizableVector<ScriptData>;
class SIR_ENGINE_API ScriptingContext final {

public:
  ScriptingContext()
      : m_nameToScript(RESERVE_SIZE), m_dynamicallyRegisteredFunctions(400),
        m_userLoadedScripts(RESERVE_SIZE), m_internalScripts(RESERVE_SIZE) {}
  ~ScriptingContext();

  bool init(bool verbose = false);
  ScriptHandle loadScript(const char *path, bool execute);
  void loadScriptsInFolder(const char *path);
  void registerCFunction(const char *name, lua_CFunction function);
  // Small hack to reload original script files and not the one in the game
  // folder this should go away once we have a proper project folder
  void reloadContext(const char *offsetPath = "../../");
  // void runAnimScripts() const;
  void runScriptSlot(SCRIPT_CALLBACK_SLOT slot) const;
  void executeFromHandle(const ScriptHandle handle) const;
  const ScriptData &getScriptDataFromHandle(const ScriptHandle handle) const {
    assert(handle.isHandleValid());
    const int id = getIndexFromHandle(handle);
    return m_userLoadedScripts[id];
  }

  ScriptingContext(const ScriptingContext &) = delete;
  ScriptingContext &operator=(const ScriptingContext &) = delete;

  inline lua_State *getContext() const { return m_state; }

private:
  void loadAllInternalScripts();
  bool initContext();
  void cleanup();
  ScriptHandle loadScriptInternal(const char *path, const bool execute);

private:
  lua_State *m_state = nullptr;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  uint32_t MAGIC_INTERNAL_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  HashMap<const char *, ScriptHandle, hashString32> m_nameToScript;
  HashMap<const char *, lua_CFunction, hashString32>
      m_dynamicallyRegisteredFunctions;
  ResizableVector<ScriptData> m_userLoadedScripts;
  ResizableVector<ScriptData> m_internalScripts;
  // script handles
  ScriptHandle
      m_callbackHandles[static_cast<uint32_t>(SCRIPT_CALLBACK_SLOT::COUNT)];
};
} // namespace SirEngine