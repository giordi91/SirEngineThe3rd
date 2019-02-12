#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/assetManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
SimpleForward::SimpleForward(const char *name) : GraphNode(name) {
  // lets create the plugs
  Plug outTexture;
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
  registerPlug(outTexture);

  // lets create the plugs
  Plug matrices;
  matrices.plugValue = 0;
  matrices.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
  matrices.nodePtr = this;
  matrices.name = "matrices";
  registerPlug(matrices);

  Plug meshes;
  meshes.plugValue = 0;
  meshes.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_MESHES;
  meshes.nodePtr = this;
  meshes.name = "meshes";
  registerPlug(meshes);

  Plug materials;
  materials.plugValue = 0;
  materials.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
  materials.nodePtr = this;
  materials.name = "materials";
  registerPlug(materials);
}

void SimpleForward::compute() {
  // meshes connections
  auto &meshConn = m_connections[&m_inputPlugs[1]];
  assert(meshConn.size() == 1 && "too many input connections");
  Plug *sourceMeshs = meshConn[0];
  AssetDataHandle meshH;
  meshH.handle = sourceMeshs->plugValue;
  uint32_t meshCount = 0;
  const dx12::MeshRuntime *meshes =
      dx12::ASSET_MANAGER->getRuntimeMeshesFromHandle(meshH, meshCount);

  // get materials
  auto &matsConn = m_connections[&m_inputPlugs[2]];
  assert(matsConn.size() == 1 && "too many input connections");
  Plug *sourceMats = matsConn[0];
  AssetDataHandle matsH;
  matsH.handle = sourceMats->plugValue;
  uint32_t matsCount = 0;
  const MaterialRuntime *mats =
      dx12::ASSET_MANAGER->getRuntimeMaterialsFromHandle(matsH, matsCount);

  assert(matsCount == meshCount);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  for (uint32_t i = 0; i < meshCount; ++i) {

    commandList->SetGraphicsRootDescriptorTable(1, mats[i].albedo);
    dx12::MESH_MANAGER->bindMeshRuntimeAndRender(meshes[i], currentFc);
  }
}
} // namespace SirEngine
