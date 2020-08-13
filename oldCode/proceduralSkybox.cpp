#include "SirEngine/graphics/nodes/proceduralSkybox.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine {

static const char *SKYBOX_RS = "proceduralSkybox_RS";
static const char *SKYBOX_PSO = "proceduralSkybox_PSO";

ProceduralSkyBoxPass::ProceduralSkyBoxPass(GraphAllocators &allocators)
    : GNode("ProceduralSkyBoxPass", "ProceduralSkyBoxPass", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &fullscreenPass = m_inputPlugs[getPlugIndex(PLUGS::FULLSCREEN_PASS)];
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";

  GPlug &depthBuffer = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &buffer = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
  buffer.plugValue = 0;
  buffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "outTexture";
}

void ProceduralSkyBoxPass::initialize() {
  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SKYBOX_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);
}

void ProceduralSkyBoxPass::compute() {

  annotateGraphicsBegin("Procedural Skybox");

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  dx12::PSO_MANAGER->bindPSO(pso, commandList);

  D3D12_RESOURCE_BARRIER barriers[5];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      inputDepthHandle, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      inputRTHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(inputRTHandle, inputDepthHandle);
  commandList->SetGraphicsRootSignature(rs);

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->DrawInstanced(6, 1, 0, 0);
  annotateGraphicsEnd();
}

void ProceduralSkyBoxPass::onResizeEvent(int, int) {
  clear();
  initialize();
}

void ProceduralSkyBoxPass::populateNodePorts() {

  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, FULLSCREEN_PASS);

  inputDepthHandle = getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);
  m_outputPlugs[0].plugValue = inputRTHandle.handle;
}
} // namespace SirEngine