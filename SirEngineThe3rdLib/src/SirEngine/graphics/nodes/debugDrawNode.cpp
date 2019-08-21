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

  Plug inDepth;
  inDepth.plugValue = 0;
  inDepth.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inDepth.nodePtr = this;
  inDepth.name = "depthTexture";
  registerPlug(inDepth);

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

  auto &conn2 = m_connections[&m_inputPlugs[1]];
  assert(conn2.size() == 1 && "too many input connections");
  Plug *sourceDepth = conn2[0];
  TextureHandle depthH{sourceDepth->plugValue};

  dx12::DEBUG_RENDERER->render(texH,depthH);

  m_outputPlugs[0].plugValue = texH.handle;
  annotateGraphicsEnd();
}
} // namespace SirEngine
