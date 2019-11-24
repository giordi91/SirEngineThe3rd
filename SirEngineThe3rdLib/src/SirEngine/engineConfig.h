#pragma once
#include "graphics/graphicsDefines.h"

namespace SirEngine {


struct EngineConfig {
  bool m_verboseStartup;

  ADAPTER_VENDOR m_adapterVendor;
  bool m_vendorTolerant;
  ADAPTER_SELECTION_RULE m_adapterSelectionRule;

  const char *m_dataSourcePath;
  const char *m_startScenePath;
  GRAPHIC_API m_graphicsAPI;

  const char *m_windowTitle;
  int m_windowWidth;
  int m_windowHeight;
  bool m_useCachedPSO;
};

struct EngineInitializationConfig {
  bool initCoreWithNoConfig = false;
  int stringPoolSizeInMB = 20 * 1024 * 1024;
  int frameAllocatorSizeInMB = 20 * 1024 * 1024;
  int persistentAllocatorInMB = 20 * 1024 * 1024;
  const char *configPath = "";
};

void parseConfigFile(const char *path, EngineConfig &config);
void initializeConfigDefault(EngineConfig &config);

void initializeEngine(const EngineInitializationConfig &config);

} // namespace SirEngine