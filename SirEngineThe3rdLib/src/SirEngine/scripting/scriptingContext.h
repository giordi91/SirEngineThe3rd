#pragma once
#include <cstdint>

// forward declare
struct lua_State;
namespace SirEngine {
class ScriptingContext final {

public:
  ScriptingContext() = default;
  ~ScriptingContext();

  void init();
  void loadScript(const char *path);
  void loadScriptsInFolder(const char *path);

  ScriptingContext(const ScriptingContext &) = delete;
  ScriptingContext &operator=(const ScriptingContext &) = delete;

private:
  lua_State *m_state = nullptr;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
};
} // namespace SirEngine