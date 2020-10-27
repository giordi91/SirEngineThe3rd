#include "SirEngine/layers/imguiDebugLayer.h"
#include "SirEngine/application.h"
#include "SirEngine/layers/graphicsLayer.h"
#include "SirEngine/log.h"
#include "io/editorPath.h"
#include "layers/editorLayer.h"

// this is needed to generate the main function, so make sure it is included
// even if intellisense say is not being used
#include "SirEngine/entryPoint.h"

class EditorApp final : public SirEngine::Application {
 public:
  EditorApp() : Application() {
    const char *projectPath = SirEngine::Editor::io::getProjectPath();
    if (projectPath != nullptr) {
      SE_CORE_INFO("Loaing project from: {}", projectPath);
    } else {
      SE_CORE_ERROR(
          "No project path provided for the editor to start up... closing");
      exit(1);
    }

    // imGuiLayer = new SirEngine::ImguiLayer();
    imGuiLayer = new SirEngine::EditorLayer();
    graphicsLayer = new SirEngine::GraphicsLayer();
    m_layerStack.pushLayer(graphicsLayer);
    m_layerStack.pushLayer(imGuiLayer);
  }
  ~EditorApp() = default;

  EditorApp(const EditorApp &) = delete;
  EditorApp &operator=(const EditorApp &) = delete;
  EditorApp(EditorApp &&) = delete;
  EditorApp &operator=(EditorApp &&) = delete;
};

SirEngine::Application *SirEngine::createApplication() {
  return new EditorApp();
}