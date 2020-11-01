#include "editor.h"

#include "SirEngine/layers/graphicsLayer.h"
#include "SirEngine/layers/imguiDebugLayer.h"
#include "SirEngine/log.h"
#include "editorGlobal.h"
#include "io/editorPath.h"
#include "layers/editorLayer.h"
#include "project.h"

// this is needed to generate the main function, so make sure it is included
// even if intellisense say is not being used
#include "SirEngine/entryPoint.h"

bool startUpEditor() {
  const char *projectPath = SirEngine::Editor::io::getProjectPath();

  if (projectPath != nullptr) {
    SE_CORE_INFO("Loaing project from: {}", projectPath);
  } else {
    SE_CORE_ERROR(
        "No project path provided for the editor to start up... closing");
    exit(1);
  }

  Editor::globals::CURRENT_PROJECT = new Editor::Project();
  bool result = Editor::globals::CURRENT_PROJECT->initilize(projectPath);
  return result;
}

void editorShutdown() {
  Editor::globals::CURRENT_PROJECT->cleanup();
  delete Editor::globals::CURRENT_PROJECT;
}

EditorApp::EditorApp() : Application() {
  bool result = startUpEditor();
  if (!result) {
    exit(1);
  }

  // imGuiLayer = new SirEngine::ImguiLayer();
  imGuiLayer = new SirEngine::EditorLayer();
  graphicsLayer = new SirEngine::GraphicsLayer();
  m_layerStack.pushLayer(graphicsLayer);
  m_layerStack.pushLayer(imGuiLayer);
}

EditorApp::~EditorApp() { editorShutdown(); }

SirEngine::Application *SirEngine::createApplication() {
  return new EditorApp();
}
