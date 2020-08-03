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

  ImGui::Text("Tile config");
  ImGui::DragFloat3("Origin", &(grassConfig->gridOrigin.x));
  ImGui::SliderFloat("Tile width", &(grassConfig->tileSize), 0.0001f, 100.0f);
  ImGui::SliderInt("Tiles per side", &grassConfig->tilesPerSide, 1, 60);
  ImGui::SliderInt("Points per tile", &grassConfig->pointsPerTile, 100, 2000);

  ImGui::Text("Blade config");
  ImGui::SliderFloat("Blade forward", &(grassConfig->bladeForward), 0.0001f,
                     5.0f);
  ImGui::SliderFloat("Blade curvature ", &(grassConfig->bladeCurvatureAmount),
                     0.0001f, 5.0f);
  ImGui::SliderFloat("Grass Bend", &(grassConfig->grassBend), 0.0001f, 1.0f);
  ImGui::SliderFloat("Width", &(grassConfig->width), 0.0001f, 0.3f);
  ImGui::SliderFloat("Height", &(grassConfig->height), 0.0001f, 4.0f);
  ImGui::SliderFloat("Width Random", &(grassConfig->widthRandom), 0.0001f,
                     0.3f);
  ImGui::SliderFloat("Height Random", &(grassConfig->heightRandom), 0.0001f,
                     2.0f);

  ImGui::Text("Wind config");
  ImGui::SliderFloat("Wind strength", &(grassConfig->windStrength), 0.0001f,
                     1.0f);
  ImGui::DragFloat2("Wind Frequency", &(grassConfig->windFrequency.x),0.01,-1,1);
  ImGui::Text("Shading config");
  ImGui::ColorPicker3("Base color", &(grassConfig->baseColor.x));
  ImGui::ColorPicker3("Tip color", &(grassConfig->tipColor.x));
  ImGui::SliderFloat("Metalness", &(grassConfig->metalness), 0.0000f,
                     1.0f);
  ImGui::SliderFloat("Roughness", &(grassConfig->roughness), 0.0000f,
                     1.0f);

  ImGui::End();
}
}  // namespace SirEngine::debug
