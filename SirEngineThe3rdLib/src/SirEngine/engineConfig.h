#pragma once

namespace SirEngine {

enum class GRAPHIC_API { DX12 = 0, VULKAN = 1,UNKNOWN };

struct EngineConfig {
  const char *m_dataSourcePath;
  const char *m_startScenePath;
  GRAPHIC_API m_graphicsAPI;
  const char* m_windowTitle;
  int m_windowWidth;
  int m_windowHeight;
};

void parseConfigFile(const char *path, EngineConfig &config);
void initializeConfigDefault(EngineConfig &config);

} // namespace SirEngine