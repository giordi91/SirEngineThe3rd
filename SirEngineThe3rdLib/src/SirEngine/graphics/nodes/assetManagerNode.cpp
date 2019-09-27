#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/assetManager.h"

namespace SirEngine {
AssetManagerNode::AssetManagerNode(GraphAllocators &allocators)
    : GNode("AssetManagerNode", "AssetManagerNode", allocators) {
  // lets create the plugs
  defaultInitializePlugsAndConnections(0, 1);

  GPlug &stream = m_outputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
  stream.plugValue = 0;
  stream.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
  stream.nodePtr = this;
  stream.name = "assetStream";
}

void AssetManagerNode::compute() {
  // here we need to get handles to the data
  m_outputPlugs[0].plugValue =
      globals::ASSET_MANAGER->getMainStreamHandle().handle;
}
} // namespace SirEngine
