#pragma once
#include <cstdint>

namespace SirEngine {
namespace Hardware {
class AMDGPUQuery;
}
namespace debug {
struct HWInfoWidget final {
  static const int NUMBER_OF_SAMPLES = 200;
  HWInfoWidget();
  void render();
  float temperatureSamples[NUMBER_OF_SAMPLES]{};
  float coreClockSamples[NUMBER_OF_SAMPLES]{};
  float memClockSamples[NUMBER_OF_SAMPLES]{};
  float usageSamples[NUMBER_OF_SAMPLES]{};
  uint32_t runningCounter = 0;

  Hardware::AMDGPUQuery* gpuQuery;
};
}; // namespace debug
} // namespace SirEngine
