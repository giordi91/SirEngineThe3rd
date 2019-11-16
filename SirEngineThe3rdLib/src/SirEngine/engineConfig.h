#pragma once

namespace SirEngine {

enum class GRAPHIC_API { DX12 = 0, VULKAN = 1, UNKNOWN };

struct EngineConfig {
  const char *m_dataSourcePath;
  const char *m_startScenePath;
  GRAPHIC_API m_graphicsAPI;
  const char *m_windowTitle;
  int m_windowWidth;
  int m_windowHeight;
  bool m_verboseStartup;
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