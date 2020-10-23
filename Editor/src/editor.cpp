#include "SirEngine/Layers/imguiDebugLayer.h"
#include "SirEngine/application.h"
#include "SirEngine/layers/graphicsLayer.h"
#include "SirEngine/log.h"
#include "layers/editorLayer.h"

// this is needed to generate the main function, so make sure it is included
// even if intellisense say is not being used
#include "SirEngine/entryPoint.h"

class Editor final : public SirEngine::Application {
 public:
  Editor() : Application() {
    // imGuiLayer = new SirEngine::ImguiLayer();
    imGuiLayer = new SirEngine::EditorLayer();
    graphicsLayer = new SirEngine::GraphicsLayer();
    m_layerStack.pushLayer(graphicsLayer);
    m_layerStack.pushLayer(imGuiLayer);
  }
  ~Editor() = default;
};

SirEngine::Application* SirEngine::createApplication() { return new Editor(); }