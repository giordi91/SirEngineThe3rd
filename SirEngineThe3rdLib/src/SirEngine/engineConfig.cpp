#include "SirEngine/engineConfig.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"

static std::string CONFIG_DATA_SOURCE_KEY = "dataSource";
static std::string CONFIG_STARTING_SCENE_KEY = "startingScene";
static std::string CONFIG_GRAPHIC_API = "graphicAPI";
static std::string CONFIG_WINDOW_TITLE = "windowTitle";
static std::string CONFIG_WINDOW_WIDTH = "windowWidth";
static std::string CONFIG_WINDOW_HEIGHT = "windowHeight";
static std::string CONFIG_ALLOCATOR_STRING_POOL = "stringPoolSizeInMB";
static std::string CONFIG_ALLOCATOR_FRAME = "frameAllocatorSizeInMB";
static std::string CONFIG_ALLOCATOR_PERSISTENT = "persistentAllocatorSizeInMB";

static std::string DEFAULT_STRING = "";

static const int DEFAULT_ALLOCATOR_STRING_POOL_SIZE_MB = 20;
static const int DEFAULT_ALLOCATOR_FRAME_SIZE_MB = 20;
static const int DEFAULT_ALLOCAOTR_PERSISTENT_SIZE_MB = 20;

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

void parseConfigFile(const char *path) {
  const nlohmann::json jobj = getJsonObj(path);

  constexpr int mbToBytes = 1024 * 1024;
  // first thing first we need to initialize the allocators
  const int stringPoolSize =
      getValueIfInJson(jobj, CONFIG_ALLOCATOR_STRING_POOL,
                       DEFAULT_ALLOCATOR_STRING_POOL_SIZE_MB) *
      mbToBytes;
  const int frameAllocSize = getValueIfInJson(jobj, CONFIG_ALLOCATOR_FRAME,
                                              DEFAULT_ALLOCATOR_FRAME_SIZE_MB) *
                             mbToBytes;
  const int persistentAllocSize =
      getValueIfInJson(jobj, CONFIG_ALLOCATOR_PERSISTENT,
                       DEFAULT_ALLOCAOTR_PERSISTENT_SIZE_MB) *
      mbToBytes;

  globals::STRING_POOL = new StringPool(stringPoolSize);
  globals::FRAME_ALLOCATOR = new StackAllocator();
  // TODO fix the interface to be same as other allocators
  globals::FRAME_ALLOCATOR->initialize(frameAllocSize);
  globals::PERSISTENT_ALLOCATOR = new ThreeSizesPool(persistentAllocSize);

  // start to process the config file
  globals::ENGINE_CONFIG = reinterpret_cast<EngineConfig *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(EngineConfig)));

  EngineConfig &config = *globals::ENGINE_CONFIG;
  config.m_dataSourcePath = persistentString(
      getValueIfInJson(jobj, CONFIG_DATA_SOURCE_KEY, DEFAULT_STRING).c_str());
  assert(config.m_dataSourcePath[0] != '\0');
  config.m_startScenePath = persistentString(
      getValueIfInJson(jobj, CONFIG_STARTING_SCENE_KEY, DEFAULT_STRING)
          .c_str());
  assert(config.m_startScenePath[0] != '\0');
  const std::string api =
      getValueIfInJson(jobj, CONFIG_GRAPHIC_API, DEFAULT_STRING);
  config.m_graphicsAPI = getAPIFromName(api);
  assert(config.m_graphicsAPI != GRAPHIC_API::UNKNOWN);

  // window stuff
  config.m_windowTitle = persistentString(
      getValueIfInJson(jobj, CONFIG_WINDOW_TITLE, DEFAULT_STRING).c_str());
  config.m_windowWidth = getValueIfInJson(jobj, CONFIG_WINDOW_WIDTH, -1);
  config.m_windowHeight = getValueIfInJson(jobj, CONFIG_WINDOW_HEIGHT, -1);

  assert(config.m_windowWidth != -1);
  assert(config.m_windowHeight != -1);
}

void initializeConfigDefault() {

  // initialize basic engine allocators
  constexpr int toBytes = 1024 * 1024;
  globals::STRING_POOL =
      new StringPool(DEFAULT_ALLOCATOR_STRING_POOL_SIZE_MB * toBytes);
  globals::FRAME_ALLOCATOR = new StackAllocator();
  // TODO fix the interface to be same as other allocators
  globals::FRAME_ALLOCATOR->initialize(DEFAULT_ALLOCATOR_FRAME_SIZE_MB *
                                       toBytes);
  globals::PERSISTENT_ALLOCATOR =
      new ThreeSizesPool(DEFAULT_ALLOCAOTR_PERSISTENT_SIZE_MB * toBytes);

  // start to process the config file
  globals::ENGINE_CONFIG = reinterpret_cast<EngineConfig *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(EngineConfig)));

  globals::ENGINE_CONFIG->m_dataSourcePath = "../data/";
  globals::ENGINE_CONFIG->m_startScenePath = "";
  globals::ENGINE_CONFIG->m_graphicsAPI = GRAPHIC_API::DX12;
  globals::ENGINE_CONFIG->m_windowTitle = "SirEngineThe3rd";
  globals::ENGINE_CONFIG->m_windowWidth = 1280;
  globals::ENGINE_CONFIG->m_windowHeight = 720;
}

void loadConfigFile(const EngineInitializationConfig &config) {
  const bool exists = fileExists(config.configPath);
  if (exists) {
    parseConfigFile(config.configPath);
  } else {
    initializeConfigDefault();
  }
}

void initializeEngine(const EngineInitializationConfig &config) {

  if (config.initCoreWithNoConfig) {
    globals::STRING_POOL = new StringPool(config.stringPoolSizeInMB);
    globals::FRAME_ALLOCATOR = new StackAllocator();
    // TODO fix the interface to be same as other allocators
    globals::FRAME_ALLOCATOR->initialize(config.frameAllocatorSizeInMB);
    globals::PERSISTENT_ALLOCATOR =
        new ThreeSizesPool(config.persistentAllocatorInMB);
  } else {
    loadConfigFile(config);
  }
}
} // namespace SirEngine
