#pragma once
#include "SirEngine/core.h"
#include "SirEngine/handle.h"
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
      : m_nameToScript(RESERVE_SIZE), m_handleToName(RESERVE_SIZE),
        m_dynamicallyRegisteredFunctions(400) {}
  ~ScriptingContext();

  bool init(bool verbose = false);
  ScriptHandle loadScript(const char *path, bool execute);
  void loadScriptsInFolder(const char *path);
  void registerCFunction(const char *name, lua_CFunction function);

  ScriptingContext(const ScriptingContext &) = delete;
  ScriptingContext &operator=(const ScriptingContext &) = delete;

private:
  lua_State *m_state = nullptr;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  HashMap<const char *, ScriptHandle, hashString32> m_nameToScript;
  HashMap<uint32_t, const char *, hashUint32> m_handleToName;
  HashMap<const char *, lua_CFunction, hashString32>
      m_dynamicallyRegisteredFunctions;
};
} // namespace SirEngine