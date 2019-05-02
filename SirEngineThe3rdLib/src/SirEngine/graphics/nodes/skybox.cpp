#include "SirEngine/graphics/nodes/skybox.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
SkyBoxPass::SkyBoxPass(const char *name) : GraphNode(name, "SkyBoxPass") {
  // lets create the plugs
  Plug fullscreenPass;
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";
  registerPlug(fullscreenPass);

  Plug depthBuffer;
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
  registerPlug(depthBuffer);

  Plug buffer;
  buffer.plugValue = 0;
  buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "buffer";
  registerPlug(buffer);
}

void SkyBoxPass::initialize() {
  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName("skybox_RS");
  pso = dx12::PSO_MANAGER->getComputePSOByName("skyboxPSO");

  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  dx12::resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
  dx12::MESH_MANAGER->loadMesh("../data/processed/meshes/skybox.model", 0,
                               &m_meshRuntime);
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                           &dx12::CURRENT_FRAME_RESOURCE->fc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
}

inline TextureHandle
getInputConnection(std::unordered_map<const Plug *, std::vector<Plug *>> &conns,
                   Plug *plug) {
  // TODO not super safe to do this, might be worth improving this
  auto &inConns = conns[plug];
  assert(inConns.size() == 1 && "too many input connections");
  Plug *source = inConns[0];
  auto h = TextureHandle{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void SkyBoxPass::compute() {

  TextureHandle bufferHandle =
      getInputConnection(m_connections, &m_inputPlugs[0]);
  TextureHandle depthHandle =
      getInputConnection(m_connections, &m_inputPlugs[1]);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  commandList->SetPipelineState(pso);

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

  TextureHandle skyHandle = globals::RENDERING_CONTEX->getEnviromentMapHandle();
  //TextureHandle skyHandle = globals::RENDERING_CONTEX->getEnviromentMapIrradianceHandle();
  globals::RENDERING_CONTEX->bindCameraBuffer(0);
  // commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  commandList->SetGraphicsRootDescriptorTable(
      1, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  // commandList->DrawInstanced(6, 1, 0, 0);
  dx12::MESH_MANAGER->bindMeshRuntimeAndRender(m_meshRuntime, currentFc);
  m_outputPlugs[0].plugValue = bufferHandle.handle;
}

void SkyBoxPass::clear() {}

void SkyBoxPass::resize(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
