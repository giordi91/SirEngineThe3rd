#pragma once

namespace SirEngine::debug {
struct GrassConfigWidget final {
  void initialize(){};
  void render();

  bool m_opened = false;
};
}  // namespace SirEngine::debug
