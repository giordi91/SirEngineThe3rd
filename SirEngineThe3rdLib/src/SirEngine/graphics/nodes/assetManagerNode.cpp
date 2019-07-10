#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/assetManager.h"

namespace SirEngine {
AssetManagerNode::AssetManagerNode()
    : GraphNode("AssetManagerNode", "AssetManagerNode") {
  // lets create the plugs
  Plug stream;
  stream.plugValue = 0;
  stream.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
  stream.nodePtr = this;
  stream.name = "assetStream";
  registerPlug(stream);
}

void AssetManagerNode::compute() {
  // here we need to get handles to the data
  m_outputPlugs[0].plugValue =
      globals::ASSET_MANAGER->getMainStreamHandle().handle;
}
} // namespace SirEngine
