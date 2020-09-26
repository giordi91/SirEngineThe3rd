#pragma once
#include "graphics/graphicsDefines.h"

namespace SirEngine {

struct EngineConfig {
  // pre-startup config
  int m_stringPoolSizeInMb;
  int m_frameAllocatorSizeInMb;
  int m_persistentAllocatorInMb;
  bool m_verboseStartup;

  // GPU config
  ADAPTER_VENDOR m_requestedAdapterVendor;
  ADAPTER_VENDOR m_selectdedAdapterVendor;
  bool m_vendorTolerant;
  ADAPTER_SELECTION_RULE m_adapterSelectionRule;

  // project and IO data config
  const char *m_dataSourcePath;
  const char *m_startScenePath;

  // window config
  const char *m_windowTitle;
  int m_windowWidth;
  int m_windowHeight;

  // graphics  config
  GRAPHIC_API m_graphicsAPI;
  bool m_useCachedPSO;
  uint32_t m_frameBufferingCount;
  uint32_t m_matrixBufferSize;
};

struct EngineInitializationConfig {
  bool initCoreWithNoConfig = false;
  int stringPoolSizeInMB = 20 * 1024 * 1024;
  int frameAllocatorSizeInMB = 20 * 1024 * 1024;
  int persistentAllocatorInMB = 20 * 1024 * 1024;
  const char *configPath = "";
};

void initializeEngine(const EngineInitializationConfig &config);

}  // namespace SirEngine