#include "SirEngine/graphics/nodes/debugDrawNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/debugRenderer.h"

namespace SirEngine {

DebugDrawNode::DebugDrawNode(GraphAllocators &allocators)
    : GNode("DebugDrawNode", "DebugDrawNode", allocators) {

  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &inDepth = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  inDepth.plugValue = 0;
  inDepth.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inDepth.nodePtr = this;
  inDepth.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
}


inline void checkHandle(const TextureHandle input,
                        const TextureHandle handleToWriteOn) {
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
}

void DebugDrawNode::initialize() {}

void DebugDrawNode::compute() {
  // get the render texture
  annotateGraphicsBegin("DebugDrawPass");

  const auto texH =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  dx12::DEBUG_RENDERER->render(texH, depth);

  m_outputPlugs[0].plugValue = texH.handle;
  annotateGraphicsEnd();
}
} // namespace SirEngine
