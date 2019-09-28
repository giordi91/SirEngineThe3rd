#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/globals.h"
#include "SirEngine/handle.h"
#include "SirEngine/textureManager.h"

#include "SirEngine/graphics/debugAnnotations.h"

namespace SirEngine {

PostProcessStack::PostProcessStack(GraphAllocators &allocators)
    : GNode("PostProcessStack", "PostProcessStack", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &depthTexture = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthTexture.plugValue = 0;
  depthTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthTexture.nodePtr = this;
  depthTexture.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
}

void PostProcessStack::initialize() {
  // initialize all layers
  const size_t stackSize = m_stack.size();
  for (size_t i = 0; i < stackSize; ++i) {
    m_stack[i]->initialize();
  }

  // allocate ping pong textures
  handles[0] = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT,
      RenderTargetFormat::R16G16B16A16_FLOAT, "postProcess1");
  handles[1] = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT,
      RenderTargetFormat::R16G16B16A16_FLOAT, "postProcess2");
}

template <typename T>
inline T getInputConnection(ResizableVector<const GPlug *> **conns,
                            const int plugId) {
  const auto conn = conns[PLUG_INDEX(plugId)];

  // TODO not super safe to do this, might be worth improving this
  assert(conn->size() == 1 && "too many input connections");
  const GPlug *source = (*conn)[0];
  const auto h = T{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void PostProcessStack::compute() {
  annotateGraphicsBegin("Post processing");

  const auto texH =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  if (m_stack.empty()) {
    m_outputPlugs[0].plugValue = texH.handle;
    return;
  }
  m_internalCounter = 0;
  const size_t stackSize = m_stack.size();

  const PostProcessResources resources{depth};
  m_stack[0]->render(texH, handles[0], resources);

  for (size_t i = 1; i < stackSize; ++i) {
    const int previous = m_internalCounter;
    m_internalCounter = (m_internalCounter + 1) % 2;
    m_stack[i]->render(handles[previous], handles[m_internalCounter],
                       resources);
  }

  m_outputPlugs[0].plugValue = handles[m_internalCounter].handle;
  annotateGraphicsEnd();
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
void PostProcessStack::onResizeEvent(int, int) {
  clear();
  initialize();
}

} // namespace SirEngine
