#include "SirEngine/graphics/nodes/proceduralSkybox.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {

static const char *SKYBOX_RS = "proceduralSkybox_RS";
static const char *SKYBOX_PSO = "proceduralSkybox_PSO";

ProceduralSkyBoxPass::ProceduralSkyBoxPass(GraphAllocators &allocators)
    : GNode("ProceduralSkyBoxPass", "ProceduralSkyBoxPass", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &fullscreenPass= m_inputPlugs[PLUG_INDEX(PLUGS::FULLSCREEN_PASS)];
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";

  GPlug &depthBuffer= m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &buffer = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  buffer.plugValue = 0;
  buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "outTexture";
}

void ProceduralSkyBoxPass::initialize() {
  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SKYBOX_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);
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

  annotateGraphicsBegin("Procedural Skybox");

  const auto bufferHandle=
      getInputConnection<TextureHandle>(m_inConnections, FULLSCREEN_PASS);

  const auto depthHandle =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

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

  globals::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->DrawInstanced(6, 1, 0, 0);
  m_outputPlugs[0].plugValue = bufferHandle.handle;
  annotateGraphicsEnd();
}

void ProceduralSkyBoxPass::clear() {}

void ProceduralSkyBoxPass::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
