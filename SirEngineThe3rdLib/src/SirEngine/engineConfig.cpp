#include "SirEngine/engineConfig.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"

static std::string CONFIG_DATA_SOURCE_KEY = "dataSource";
static std::string CONFIG_STARTING_SCENE_KEY = "startingScene";
static std::string CONFIG_GRAPHIC_API = "graphicAPI";
static std::string CONFIG_WINDOW_TITLE= "windowTitle";
static std::string CONFIG_WINDOW_WIDTH= "windowWidth";
static std::string CONFIG_WINDOW_HEIGHT= "windowHeight";
static std::string DEFAULT_STRING = "";

static const char *GRAPHICS_API_TO_NAME[] = {"DirectX 12", "Vulkan"};
static const std::unordered_map<std::string, SirEngine::GRAPHIC_API>
    NAME_TO_GRAPHIC_API{
        {"Vulkan", SirEngine::GRAPHIC_API::VULKAN},
        {"DX12", SirEngine::GRAPHIC_API::DX12},
    };

namespace SirEngine {

SirEngine::GRAPHIC_API getAPIFromName(const std::string &apiName) {
	const auto found = NAME_TO_GRAPHIC_API.find(apiName);
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
  const std::string api = getValueIfInJson(jobj, CONFIG_GRAPHIC_API, DEFAULT_STRING);
  config.m_graphicsAPI = getAPIFromName(api);
  assert(config.m_graphicsAPI != GRAPHIC_API::UNKNOWN);

  //window stuff
  config.m_windowTitle = persistentString(
      getValueIfInJson(jobj, CONFIG_WINDOW_TITLE, DEFAULT_STRING).c_str());
  config.m_windowWidth = 
      getValueIfInJson(jobj, CONFIG_WINDOW_WIDTH, -1);
  config.m_windowHeight= 
      getValueIfInJson(jobj, CONFIG_WINDOW_HEIGHT, -1);

  assert(config.m_windowWidth !=-1);
  assert(config.m_windowHeight!=-1);
}

void initializeConfigDefault(EngineConfig &config) {
  config.m_dataSourcePath = "../data/";
  config.m_startScenePath = "";
  config.m_graphicsAPI = GRAPHIC_API::DX12;
  config.m_windowTitle= "SirEngineThe3rd";
  config.m_windowWidth = 1280;
  config.m_windowHeight= 720;
}
} // namespace SirEngine
