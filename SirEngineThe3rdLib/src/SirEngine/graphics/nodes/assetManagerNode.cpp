#include "SirEngine/graphics/nodes/assetmanagerNode.h"

namespace SirEngine {
AssetManagerNode::AssetManagerNode() : GraphNode("AssetManagerNode") {
  // lets create the plugs
  Plug matrices;
  matrices.plugValue = 0;
  matrices.flags = PlugFlags::OUTPUT | PlugFlags::CPU_BUFFER;
  matrices.nodePtr = this;
  matrices.name = "matrices";
  registerPlug(matrices);

  Plug meshes;
  meshes.plugValue = 0;
  meshes.flags = PlugFlags::OUTPUT | PlugFlags::MESHES;
  meshes.nodePtr = this;
  meshes.name = "meshes";
  registerPlug(meshes);

  Plug materials;
  materials.plugValue = 0;
  materials.flags = PlugFlags::OUTPUT | PlugFlags::CPU_BUFFER;
  materials.nodePtr = this;
  materials.name = "materials";
  registerPlug(materials);
}
} // namespace SirEngine
