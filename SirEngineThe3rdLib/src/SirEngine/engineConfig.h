#pragma once

namespace SirEngine {

enum class GRAPHIC_API { DX12 = 0, VULKAN = 1,UNKNOWN };

struct EngineConfig {
  const char *m_dataSourcePath;
  const char *m_startScenePath;
  GRAPHIC_API m_graphicsAPI;
};

void parseConfigFile(const char *path, EngineConfig &config);
void initializeConfigDefault(EngineConfig &config);

} // namespace SirEngine