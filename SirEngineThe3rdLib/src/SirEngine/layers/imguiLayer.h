#pragma once

#include "SirEngine/layer.h"

namespace SirEngine {

namespace dx12 {
struct D3DBuffer;
}
class SIR_ENGINE_API ImguiLayer : public Layer {
public:
  ImguiLayer() : Layer("ImGuiLayer") {}
  ~ImguiLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;

private:
	dx12::D3DBuffer *m_fontTextureDescriptor = nullptr;
	int m_descriptorIndex;
};
} // namespace SirEngine
