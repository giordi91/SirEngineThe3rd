#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/globals.h"
#include "SirEngine/handle.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {

PostProcessStack::PostProcessStack()
    : GraphNode("PostProcessStack", "PostProcessStack") {

  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);

  Plug outTexture;
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
  registerPlug(outTexture);
}

void PostProcessStack::initialize() {

  // initialize all layers
  size_t stackSize = m_stack.size();
  for (size_t i = 0; i < stackSize; ++i) {
    m_stack[i]->initialize();
  }

  // allocate ping pong textures
  handles[0] = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "postProcess1");
  handles[1] = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "postProcess2");
}

void PostProcessStack::compute() {
  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH;
  texH.handle = source->plugValue;

  if (m_stack.empty()) {
    m_outputPlugs[0].plugValue = texH.handle;
    return;
  }
  m_internalCounter = 0;
  size_t stackSize = m_stack.size();
  m_stack[0]->render(texH, handles[0]);

  for (size_t i = 1; i < stackSize; ++i) {
    int previous = m_internalCounter;
    m_internalCounter = (m_internalCounter + 1) % 2;
    m_stack[i]->render(handles[previous], handles[m_internalCounter]);
  }

  m_outputPlugs[0].plugValue = handles[m_internalCounter].handle;
}

void PostProcessStack::clear() {
  if (handles[0].isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handles[0]);
    handles[0].handle = 0;
  }
  if (handles[1].isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handles[1]);
    handles[1].handle = 0;
  }
}
void PostProcessStack::resize(int screenWidth, int screenHeight) {
  clear();
  initialize();
}

} // namespace SirEngine
