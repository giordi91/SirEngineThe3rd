#pragma once
#include <chrono>

namespace SirEngine {
namespace Hardware {
class AMDGPUQuery {
public:
  AMDGPUQuery() = default;
  ~AMDGPUQuery();

  // delta time defines how often will a new sample be taken
  void update();
  bool initialize(int deltaTimeInMS);

public:
  float m_temp = 0.0f;

  int m_coreFreq = 0;
  int m_minCoreFreq = 0;
  int m_maxCoreFreq = 0;

  int m_memFreq = 0;
  int m_minMemFreq = 0;
  int m_maxMemFreq = 0;

  int m_usage = 0;
  int m_adapterId = -1;
  bool m_invalid = false;

  int m_fanSpeed;
  int m_maxFanSpeed;
  int m_minFanSpeed;

  int m_deltaTimeInMS;

  std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdate;
};
} // namespace Hardware
} // namespace SirEngine
