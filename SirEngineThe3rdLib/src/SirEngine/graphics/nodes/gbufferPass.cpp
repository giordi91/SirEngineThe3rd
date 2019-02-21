#include "SirEngine/graphics/nodes/gbufferPass.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
GBufferPass::GBufferPass(const char *name) : GraphNode(name, "GBufferPass") {
  // lets create the plugs
  Plug geometryBuffer;
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "geometry";
  registerPlug(geometryBuffer);

  Plug normalBuffer;
  normalBuffer.plugValue = 0;
  normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  normalBuffer.nodePtr = this;
  normalBuffer.name = "normal";
  registerPlug(normalBuffer);

  Plug specularBuffer;
  specularBuffer.plugValue = 0;
  specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  specularBuffer.nodePtr = this;
  specularBuffer.name = "specular";
  registerPlug(specularBuffer);

  Plug depthBuffer;
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
  registerPlug(depthBuffer);

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

  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName("gbufferRS");
  pso = dx12::PSO_MANAGER->getComputePSOByName("gbufferPSO");
}

void GBufferPass::initialize() {

  m_depth = dx12::TEXTURE_MANAGER->createDepthTexture(
      "gbufferDepth", globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);

  m_geometryBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "geometryBuffer");

  m_normalBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT,
      RenderTargetFormat::R11G11B10, "normalBuffer");

  m_specularBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "specularBuffer");
}

void GBufferPass::compute() {
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

  commandList->SetPipelineState(pso);

  D3D12_RESOURCE_BARRIER barriers[4];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_geometryBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_normalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_specularBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->clearDepth(m_depth);
  float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  globals::TEXTURE_MANAGER->clearRT(m_geometryBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_normalBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_specularBuffer, color);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[3] = {
      dx12::TEXTURE_MANAGER->getRTVDx12(m_geometryBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_normalBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_specularBuffer).cpuHandle};

  auto depthDescriptor = dx12::TEXTURE_MANAGER->getRTVDx12(m_depth).cpuHandle;
  commandList->OMSetRenderTargets(3, handles, true, &depthDescriptor);
  //
  commandList->SetGraphicsRootSignature(rs);
  globals::RENDERING_CONTEX->bindCameraBuffer(0);

  for (uint32_t i = 0; i < meshCount; ++i) {

    // commandList->SetGraphicsRootDescriptorTable(1, mats[i].albedo);
    // commandList->SetGraphicsRootDescriptorTable(1, mats[i].albedo);
    commandList->SetGraphicsRootConstantBufferView(1, mats[i].cbVirtualAddress);
    dx12::MESH_MANAGER->bindMeshRuntimeAndRender(meshes[i], currentFc);
  }

  m_outputPlugs[0].plugValue = m_geometryBuffer.handle;
  m_outputPlugs[1].plugValue = m_geometryBuffer.handle;
  m_outputPlugs[2].plugValue = m_specularBuffer.handle;
  m_outputPlugs[3].plugValue = m_depth.handle;
}

#define FREE_TEXTURE_IF_VALID(h)                                               \
  if (h.isHandleValid()) {                                                     \
    dx12::TEXTURE_MANAGER->free(h);                                            \
    h.handle = 0;                                                              \
  }

void GBufferPass::clear() {
  FREE_TEXTURE_IF_VALID(m_depth)
  FREE_TEXTURE_IF_VALID(m_geometryBuffer)
  FREE_TEXTURE_IF_VALID(m_normalBuffer)
  FREE_TEXTURE_IF_VALID(m_specularBuffer)
}

void GBufferPass::resize(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
