
#include "SirEngine/graphics/nodes/FinalBlitNode.h"

namespace SirEngine {
FinalBlitNode::FinalBlitNode() : GraphNode("FinalBlit") {
  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::INPUT | PlugFlags::TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);
}
} // namespace SirEngine
