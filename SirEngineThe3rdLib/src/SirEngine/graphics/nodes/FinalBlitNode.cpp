
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine {

FinalBlitNode::FinalBlitNode() : GraphNode("FinalBlit") {
  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);
}

void FinalBlitNode::compute() {
  // get the render texture

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH;
  texH.handle = source->plugValue;


  TextureHandle destination  = dx12::SWAP_CHAIN->currentBackBufferTexture();
  globals::TEXTURE_MANAGER->copyTexture(texH,destination);
}
} // namespace SirEngine
