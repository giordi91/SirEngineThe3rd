#include "SirEngine/graphics/nodes/debugDrawNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/debugRenderer.h"

namespace SirEngine {


DebugDrawNode::DebugDrawNode(const char *name) : GraphNode(name, "DebugDrawNode") {
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

inline void checkHandle(const TextureHandle input,
                        const TextureHandle handleToWriteOn) {
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
}


void DebugDrawNode::initialize() {
}

void DebugDrawNode::compute() {
  // get the render texture
  annotateGraphicsBegin("DebugDrawPass");

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH{source->plugValue};

  dx12::DEBUG_RENDERER->render();

  m_outputPlugs[0].plugValue = texH.handle;
  annotateGraphicsEnd();
}
} // namespace SirEngine
