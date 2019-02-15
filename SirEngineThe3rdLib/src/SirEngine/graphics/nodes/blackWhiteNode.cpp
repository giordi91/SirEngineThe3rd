#include "SirEngine/graphics/nodes/blackWhiteNode.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine {

BlackWhiteNode::BlackWhiteNode(const char* name) : GraphNode(name) {
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

void BlackWhiteNode::compute() {
  // get the render texture

auto &conn = m_connections[&m_inputPlugs[0]];
assert(conn.size() == 1 && "too many input connections");
Plug *source = conn[0];
TextureHandle texH;
texH.handle = source->plugValue;

m_outputPlugs[0].plugValue = texH.handle;

}
} // namespace SirEngine
