#include "SirEngine/engineConfig.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"

static std::string CONFIG_DATA_SOURCE_KEY = "dataSource";
static std::string CONFIG_STARTING_SCENE_KEY = "startingScene";
static std::string CONFIG_GRAPHIC_API = "graphicAPI";
static std::string DEFAULT_STRING = "";

static const char *GRAPHICS_API_TO_NAME[] = {"DirectX 12", "Vulkan"};
static const std::unordered_map<std::string, SirEngine::GRAPHIC_API>
    NAME_TO_GRAPHIC_API{
        {"Vulkan", SirEngine::GRAPHIC_API::VULKAN},
        {"DX12", SirEngine::GRAPHIC_API::DX12},
    };

namespace SirEngine {

SirEngine::GRAPHIC_API getAPIFromName(const std::string &apiName) {
  auto found = NAME_TO_GRAPHIC_API.find(apiName);
  if (found != NAME_TO_GRAPHIC_API.end()) {
    return found->second;
  }
  return GRAPHIC_API::UNKNOWN;
}

void parseConfigFile(const char *path, EngineConfig &config) {
  const nlohmann::json jobj = getJsonObj(path);

  config.m_dataSourcePath = persistentString(
      getValueIfInJson(jobj, CONFIG_DATA_SOURCE_KEY, DEFAULT_STRING).c_str());
  assert(config.m_dataSourcePath[0] != '\0');
  config.m_startScenePath = persistentString(
      getValueIfInJson(jobj, CONFIG_STARTING_SCENE_KEY, DEFAULT_STRING)
          .c_str());
  assert(config.m_startScenePath[0] != '\0');
  std::string api = getValueIfInJson(jobj, CONFIG_GRAPHIC_API, DEFAULT_STRING);
  config.m_graphicsAPI = getAPIFromName(api);
  assert(config.m_graphicsAPI != GRAPHIC_API::UNKNOWN);
}

void initializeConfigDefault(EngineConfig &config) {
  config.m_dataSourcePath = "../data/";
  config.m_startScenePath = "";
  config.m_graphicsAPI = GRAPHIC_API::DX12;
}
} // namespace SirEngine
