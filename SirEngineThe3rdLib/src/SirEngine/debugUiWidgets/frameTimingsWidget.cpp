#include "SirEngine/debugUiWidgets/frameTimingsWidget.h"
#include "SirEngine/globals.h"
#include <imgui/imgui.h>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include "SirEngine/log.h"


namespace SirEngine::debug
{
FrameTimingsWidget::FrameTimingsWidget() {
  for (int i = 0; i < NUMBER_OF_SAMPLES; ++i) {
    m_samples[i] = 0.0f;
  }
  for (int i = 0; i < NUMBER_OF_HISTOGRAMS_BUCKETS; ++i) {
    m_framesHistogram[i] = 0.0f;
  }

}

void FrameTimingsWidget::render() {

  std::string totalFrames{"Number of frames: "};
  totalFrames += std::to_string(globals::TOTAL_NUMBER_OF_FRAMES);
  ImGui::Text(totalFrames.c_str());
  std::string totalTime{"Time since start: "};
  totalTime += std::to_string(static_cast<float>(
                   globals::GAME_CLOCK.getDeltaFromOrigin() * 1e-9)) +
               "s";
  ImGui::Text(totalTime.c_str());
  if (!ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  float lastFrameInMs = static_cast<float>(globals::LAST_FRAME_TIME_NS) * 1e-6f;
  // render frame graphs
  m_samples[m_runningCounter] = lastFrameInMs;
  m_runningCounter = ((m_runningCounter + 1) % NUMBER_OF_SAMPLES);
  float finalSamples[NUMBER_OF_SAMPLES];
  float average = 0.0f;
  float maxV = -100.0f;
  float minV = 100.0f;

  // lets compute min max and average
  for (int i = 0; i < NUMBER_OF_SAMPLES; ++i) {
    float v = m_samples[i];
    maxV = maxV > v ? maxV : v;
    minV = minV < v ? minV : v;
    average += v;
  }
  average /= static_cast<float>(NUMBER_OF_SAMPLES);

  // final
  for (int i = 0; i < NUMBER_OF_SAMPLES; ++i) {
    int idx = (m_runningCounter + i) % NUMBER_OF_SAMPLES;
    finalSamples[i] = ((m_samples[idx] - minV) / (maxV - minV) - 0.5f) * 2.0f;
  }
  std::string overlay{"avg "};
  overlay += std::to_string(average) + " ms";
  ImGui::Text("Frame Times:");

  ImGui::PushItemWidth(ImGui::GetWindowWidth() - 90);

  //\n are tricks to try to place the bottom scale in the right place
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << maxV << "ms\n\n\n\n\n"
         << minV << "ms";
  std::string s = stream.str();
  ImGui::PlotLines(s.c_str(), finalSamples, IM_ARRAYSIZE(finalSamples), 0,
                   overlay.c_str(), -1.0f, 1.0f, ImVec2(0, 80));

  // render histogram frames
  // Use fixed width for labels (by passing a negative value), the rest
  // goes to widgets. We choose a width proportional to our font size.
  ImGui::PushItemWidth(-1);

  // computing the bucket size
  float bucketSize =
      MAX_FRAME_TIME / static_cast<float>(NUMBER_OF_HISTOGRAMS_BUCKETS - 1);
  // now we can figure out which bucket our value goes into
  auto bucket = static_cast<uint32_t>(std::floorf(lastFrameInMs / bucketSize));

  // update the histogram, we reserved last bucket for all the frames greater
  // than the max
  if (lastFrameInMs > MAX_FRAME_TIME) {
    ++m_framesHistogram[NUMBER_OF_HISTOGRAMS_BUCKETS - 1];
  } else {
    ++m_framesHistogram[bucket];
  }

  float finalHisto[NUMBER_OF_HISTOGRAMS_BUCKETS];

  // we need to normalzie the histogram based on the biggest one,
  // imgui accepts values from 0-1
  float tallestValue = 0.0f;
  for (int i = 0; i < NUMBER_OF_HISTOGRAMS_BUCKETS; ++i) {
    if (m_framesHistogram[i] > tallestValue) {
      tallestValue = m_framesHistogram[i];
    }
  }
  // now that we have it we can normalize and populate the final histogram array
  for (int i = 0; i < NUMBER_OF_HISTOGRAMS_BUCKETS; ++i) {
    finalHisto[i] = m_framesHistogram[i] / tallestValue;
  }
  std::string histoLabel{"Frame distribution: bucket size "};
  histoLabel += std::to_string(bucketSize) + "ms";
  ImGui::Text(histoLabel.c_str());
  ImGui::PlotHistogram("", finalHisto, IM_ARRAYSIZE(finalHisto), 0, NULL, 0.0f,
                       1.0f, ImVec2(0, 80));
  ImGui::PopItemWidth();
}
} // namespace SirEngine
