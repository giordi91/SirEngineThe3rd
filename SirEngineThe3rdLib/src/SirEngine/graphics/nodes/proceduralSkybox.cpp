#include "SirEngine/graphics/nodes/proceduralSkybox.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
ProceduralSkyBoxPass::ProceduralSkyBoxPass(const char *name)
    : GraphNode(name, "ProceduralSkyBoxPass") {
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

void ProceduralSkyBoxPass::initialize() {
  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      "proceduralSkybox_RS");
  pso = dx12::PSO_MANAGER->getComputePSOByName("proceduralSkybox_PSO");
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

void ProceduralSkyBoxPass::compute() {


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
      depthHandle, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers,
      counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      bufferHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers,
      counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(bufferHandle, depthHandle);
  commandList->SetGraphicsRootSignature(rs);

  globals::RENDERING_CONTEX->bindCameraBuffer(0);
  //commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  //commandList->SetGraphicsRootDescriptorTable(
  //    2, dx12::TEXTURE_MANAGER->getSRVDx12(depthHandle).gpuHandle);
  //commandList->SetGraphicsRootDescriptorTable(
  //    3, dx12::TEXTURE_MANAGER->getSRVDx12(gbufferHandle).gpuHandle);
  //commandList->SetGraphicsRootDescriptorTable(
  //    4, dx12::TEXTURE_MANAGER->getSRVDx12(normalBufferHandle).gpuHandle);
  //commandList->SetGraphicsRootDescriptorTable(
  //    5, dx12::TEXTURE_MANAGER->getSRVDx12(specularBufferHandle).gpuHandle);

  commandList->DrawInstanced(6, 1, 0, 0);
  m_outputPlugs[0].plugValue = bufferHandle.handle;
}


void ProceduralSkyBoxPass::clear() {
}

void ProceduralSkyBoxPass::resize(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
