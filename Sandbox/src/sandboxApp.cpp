#include "SirEngine/Layers/editorLayer.h"
#include "SirEngine/layers/graphicsLayer.h"


#include <SirEngine.h>

class Sandbox : public SirEngine::Application {
public:
  Sandbox(): Application()
  {
  //imGuiLayer = new ImguiLayer();
  imGuiLayer = new SirEngine::EditorLayer();
  graphicsLayer = new SirEngine::GraphicsLayer();
  m_layerStack.pushLayer(graphicsLayer);
  m_layerStack.pushLayer(imGuiLayer);
	  
  };
  ~Sandbox() = default;
};


SirEngine::Application* SirEngine::createApplication()
{
	return new Sandbox();
}