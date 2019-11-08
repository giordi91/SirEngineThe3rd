#pragma once
#include <cstdint>

namespace SirEngine {
namespace Hardware {
#ifdef BUILD_AMD
class AMDGPUQuery;
#endif
} // namespace Hardware
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

#ifdef BUILD_AMD
  Hardware::AMDGPUQuery *gpuQuery;
#endif
};
}; // namespace debug
} // namespace SirEngine
