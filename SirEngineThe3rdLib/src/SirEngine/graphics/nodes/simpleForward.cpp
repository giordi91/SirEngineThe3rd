#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/assetManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
SimpleForward::SimpleForward(const char *name)
    : GraphNode(name, "SimpleForward") {
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

void SimpleForward::initialize() {
  // we need to allocate a render target that we can use for the forward
  // dx12::TEXTURE_MANAGER->
  m_renderTarget = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "simpleForwardRT");

  // to do move function to interface
  m_depth = dx12::TEXTURE_MANAGER->createDepthTexture(
      "simpleForwardRTDepth", globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);
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
      globals::ASSET_MANAGER->getRuntimeMeshesFromHandle(meshH, meshCount);

  // get materials
  auto &matsConn = m_connections[&m_inputPlugs[2]];
  assert(matsConn.size() == 1 && "too many input connections");
  Plug *sourceMats = matsConn[0];
  AssetDataHandle matsH;
  matsH.handle = sourceMats->plugValue;
  uint32_t matsCount = 0;
  const MaterialRuntime *mats =
      globals::ASSET_MANAGER->getRuntimeMaterialsFromHandle(matsH, matsCount);

  assert(matsCount == meshCount);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->clearDepth(m_depth);
  float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  globals::TEXTURE_MANAGER->clearRT(m_renderTarget, color);
  globals::TEXTURE_MANAGER->bindRenderTarget(m_renderTarget, m_depth);

  for (uint32_t i = 0; i < meshCount; ++i) {

    commandList->SetGraphicsRootDescriptorTable(1, mats[i].albedo);
    dx12::MESH_MANAGER->bindMeshRuntimeAndRender(meshes[i], currentFc);
  }

  m_outputPlugs[0].plugValue = m_renderTarget.handle;
}

void SimpleForward::clear() {
  if (m_renderTarget.isHandleValid()) {
    dx12::TEXTURE_MANAGER->free(m_renderTarget);
    m_renderTarget.handle = 0;
  }
  if (m_depth.isHandleValid()) {
    dx12::TEXTURE_MANAGER->free(m_depth);
  }
}

void SimpleForward::resize(int screenWidth, int screenHeight)
{
	clear();
	initialize();
}
} // namespace SirEngine
