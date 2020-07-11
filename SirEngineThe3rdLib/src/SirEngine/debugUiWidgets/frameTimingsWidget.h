#pragma once
#include <cstdint>

namespace SirEngine {
namespace debug {
struct FrameTimingsWidget final {
  static const int NUMBER_OF_SAMPLES = 200;
  static const int NUMBER_OF_HISTOGRAMS_BUCKETS = 11;
#define MAX_FRAME_TIME 20.0f
  FrameTimingsWidget();
  void render();
  float samples[NUMBER_OF_SAMPLES];
  float tempSamples[NUMBER_OF_SAMPLES];
  uint32_t runningCounter = 0;
  float framesHistogram[NUMBER_OF_HISTOGRAMS_BUCKETS]{};

};
}; // namespace debug
} // namespace SirEngine
