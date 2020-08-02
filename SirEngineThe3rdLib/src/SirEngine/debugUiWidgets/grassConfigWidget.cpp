#include "SirEngine/debugUiWidgets/grassConfigWidget.h"

#include "SirEngine/globals.h"
#include "SirEngine/graphics/techniques/grass.h"
#include "SirEngine/interopData.h"
#include "imgui/imgui.h"

namespace SirEngine::debug {
void GrassConfigWidget::render() {
  void* config = globals::INTEROP_DATA->getData("grassConfig");
  if (config == nullptr) {
    return;
  }

  auto* grassConfig = static_cast<GrassConfig*>(config);
  if (!ImGui::Begin("Grass Config", &m_opened)) {
    ImGui::End();
    return;
  }

  ImGui::SliderFloat("Tile width", &(grassConfig->tileSize), 0.0001f, 100.0f);
  ImGui::SliderInt("Tiles per side", &grassConfig->tilesPerSide, 1, 60);
  ImGui::End();
}
}  // namespace SirEngine::debug
