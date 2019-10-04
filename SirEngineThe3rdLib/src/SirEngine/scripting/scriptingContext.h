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

template class SIR_ENGINE_API HashMap<const char *, ScriptHandle, hashString32>;
template class SIR_ENGINE_API HashMap<uint32_t, const char *, hashUint32>;
class SIR_ENGINE_API ScriptingContext final {

public:
  ScriptingContext()
      : m_nameToScript(RESERVE_SIZE), m_dynamicallyRegisteredFunctions(400),
        m_loadedPaths(RESERVE_SIZE) {}
  ~ScriptingContext();

  bool init(bool verbose = false);
  ScriptHandle loadScript(const char *path, bool execute);
  void loadScriptsInFolder(const char *path);
  void registerCFunction(const char *name, lua_CFunction function);
  // Small hack to reload original script files and not the one in the game
  // folder this should go away once we have a proper project folder
  void reloadContext(const char *offsetPath = "../../");
  void runAnimScripts() const;

  ScriptingContext(const ScriptingContext &) = delete;
  ScriptingContext &operator=(const ScriptingContext &) = delete;

private:
  struct ScriptData {
    const char *path;
    const char *fileName;
    bool shouldExecute;
    ScriptHandle handle;
    uint32_t magicNumber;
  };

private:
  bool initContext();
  void cleanup();

private:
  lua_State *m_state = nullptr;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  HashMap<const char *, ScriptHandle, hashString32> m_nameToScript;
  HashMap<const char *, lua_CFunction, hashString32>
      m_dynamicallyRegisteredFunctions;
  ResizableVector<ScriptData> m_loadedPaths;
  // script handles
  ScriptHandle m_runAnimHandle{};
};
} // namespace SirEngine