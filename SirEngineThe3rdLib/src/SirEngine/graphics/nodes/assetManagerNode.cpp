#include "SirEngine/graphics/nodes/assetmanagerNode.h"
#include "SirEngine/assetManager.h"

namespace SirEngine {
AssetManagerNode::AssetManagerNode()
    : GraphNode("AssetManagerNode", "AssetManagerNode") {
  // lets create the plugs
  Plug matrices;
  matrices.plugValue = 0;
  matrices.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
  matrices.nodePtr = this;
  matrices.name = "matrices";
  registerPlug(matrices);

  Plug meshes;
  meshes.plugValue = 0;
  meshes.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_MESHES;
  meshes.nodePtr = this;
  meshes.name = "meshes";
  registerPlug(meshes);

  Plug materials;
  materials.plugValue = 0;
  materials.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
  materials.nodePtr = this;
  materials.name = "materials";
  registerPlug(materials);
}

void AssetManagerNode::compute() {
  // here we need to get handles to the data
  m_outputPlugs[0].plugValue =
      globals::ASSET_MANAGER->getStaticDataHandle(AssetDataType::MATRICES)
          .handle;

  m_outputPlugs[1].plugValue =
      globals::ASSET_MANAGER->getStaticDataHandle(AssetDataType::MESHES).handle;

  m_outputPlugs[2].plugValue =
      globals::ASSET_MANAGER->getStaticDataHandle(AssetDataType::MATERIALS)
          .handle;
}
} // namespace SirEngine
