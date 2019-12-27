#include "SirEngine/graphics/nodes/skybox.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"

namespace SirEngine {
static const char *SKYBOX_RS = "skybox_RS";
static const char *SKYBOX_PSO = "skyboxPSO";

SkyBoxPass::SkyBoxPass(GraphAllocators &allocators)
    : GNode("SkyBoxPass", "SkyBoxPass", allocators) {

  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &fullscreenPass = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";

  GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &buffer = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEX)];
  buffer.plugValue = 0;
  buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "buffer";
}

void SkyBoxPass::initialize() {
  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SKYBOX_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);

  // dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  // if (!dx12::CURRENT_FRAME_RESOURCE->fc.isListOpen) {
  //  dx12::resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
  //}
  skyboxHandle = dx12::MESH_MANAGER->loadMesh(
      "../data/processed/meshes/skybox.model", true);
  // dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
  //                         &dx12::CURRENT_FRAME_RESOURCE->fc);
  // dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
}

inline TextureHandle getInputConnection(ResizableVector<const GPlug *> **conns,
                                        const int plugId) {
  const auto conn = conns[PLUG_INDEX(plugId)];

  // TODO not super safe to do this, might be worth improving this
  assert(conn->size() == 1 && "too many input connections");
  const GPlug *source = (*conn)[0];
  const auto h = TextureHandle{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void SkyBoxPass::compute() {

  annotateGraphicsBegin("Skybox");
  const TextureHandle bufferHandle =
      getInputConnection(m_inConnections, IN_TEXTURE);
  const TextureHandle depthHandle = getInputConnection(m_inConnections, DEPTH);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_VIEWPORT *currViewport = dx12::SWAP_CHAIN->getViewport();
  D3D12_VIEWPORT viewport;
  viewport.MaxDepth = 0.000000000f;
  viewport.MinDepth = 0.000000000f;
  viewport.Height = currViewport->Height;
  viewport.Width = currViewport->Width;
  viewport.TopLeftX = currViewport->TopLeftX;
  viewport.TopLeftY = currViewport->TopLeftY;
  currentFc->commandList->RSSetViewports(1, &viewport);

  dx12::PSO_MANAGER->bindPSO(pso, commandList);

  D3D12_RESOURCE_BARRIER barriers[5];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      depthHandle, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      bufferHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(bufferHandle, depthHandle);
  commandList->SetGraphicsRootSignature(rs);

  TextureHandle skyHandle = dx12::RENDERING_CONTEXT->getEnviromentMapHandle();
  // TextureHandle skyHandle =
  // globals::RENDERING_CONTEX->getEnviromentMapIrradianceHandle();
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  // commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  commandList->SetGraphicsRootDescriptorTable(
      1, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  // commandList->DrawInstanced(6, 1, 0, 0);
  //dx12::MESH_MANAGER->bindMeshRuntimeAndRender(skyboxHandle, currentFc);
  dx12::MESH_MANAGER->bindMesh(skyboxHandle, currentFc->commandList,MeshAttributeFlags::POSITIONS,2);
  dx12::MESH_MANAGER->render(skyboxHandle, currentFc);
  m_outputPlugs[0].plugValue = bufferHandle.handle;

  // reset normal viewport
  commandList->RSSetViewports(1, currViewport);
  annotateGraphicsEnd();
}

void SkyBoxPass::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
