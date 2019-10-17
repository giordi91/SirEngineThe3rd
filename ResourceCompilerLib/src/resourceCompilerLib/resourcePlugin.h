#include "core.h"
#include <Windows.h>
#include <string>
#include <unordered_map>

typedef bool (*ResourceProcessFunction)(const std::string &assetPath,
                                        const std::string &outputPath,
                                        const std::string &pluginArgs);
class PluginRegistry {
public:
  static RC_API void init();
  RC_API void clear();
  static RC_API PluginRegistry *getInstance() { return registryInst; }

  void RC_API registerFunction(const std::string &name,
                               ResourceProcessFunction func);
  RC_API void loadPlugin(const std::string &dllPath);
  RC_API ResourceProcessFunction getFunction(const std::string &name) const {
    auto found = m_registry.find(name);
    if (found != m_registry.end()) {
      return found->second;
    }
    return nullptr;
  }
  RC_API void loadPluginsInFolder(const std::string &path);

  PluginRegistry() = default;
  ~PluginRegistry() {
    clear();
    registryInst = nullptr;
  };

private:
  std::unordered_map<std::string, ResourceProcessFunction> m_registry;
  std::vector<HMODULE> m_dlls;
  static PluginRegistry *registryInst;
};

typedef bool (*RegisterPluginFunction)(PluginRegistry *registry);
