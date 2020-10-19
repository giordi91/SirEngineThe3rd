#include "core.h"
#include <Windows.h>
#include <string>
#include <unordered_map>

typedef bool (*ResourceProcessFunction)(const std::string &assetPath,
                                        const std::string &outputPath,
                                        const std::string &pluginArgs);
class PluginRegistry {
public:
  static void init();
  void clear();
  static PluginRegistry *getInstance() { return registryInst; }

  void registerFunction(const std::string &name,
                               ResourceProcessFunction func);
  void loadPlugin(const std::string &dllPath, bool verbose);
  ResourceProcessFunction getFunction(const std::string &name) const {
    auto found = m_registry.find(name);
    if (found != m_registry.end()) {
      return found->second;
    }
    return nullptr;
  }
  void loadPluginsInFolder(const std::string &path, bool verbose =false);

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
