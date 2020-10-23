#include "layers/editorLayer.h"
#include "SirEngine/layers/graphicsLayer.h"


#include "SirEngine/application.h"
#include "SirEngine/log.h"

#include "SirEngine/entryPoint.h"

class Editor final : public SirEngine::Application {
public:
  Editor(): Application()
  {
  //imGuiLayer = new ImguiLayer();
  imGuiLayer = new SirEngine::EditorLayer();
  graphicsLayer = new SirEngine::GraphicsLayer();
  m_layerStack.pushLayer(graphicsLayer);
  m_layerStack.pushLayer(imGuiLayer);
	  
  };
  ~Editor() = default;
};


SirEngine::Application* SirEngine::createApplication()
{
	return new Editor();
}