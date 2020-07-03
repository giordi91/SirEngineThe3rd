#include "SirEngine/graphics/nodes/skybox.h"

#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"

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
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(SKYBOX_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);

  skyboxHandle = globals::MESH_MANAGER->loadMesh(
      "../data/processed/meshes/skybox.model", true);

  const char *queues[5] = {nullptr, nullptr, nullptr, nullptr,
                           "skybox"};
  m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      "skybox", 0,
      queues

  );
	
  /*
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
  */
}

void SkyBoxPass::compute() {
  annotateGraphicsBegin("Skybox");

  // this will take care of binding the back buffer and the input and transition
  // both the back buffer  and input texture
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  // next we bind the material, this will among other things bind the pso and rs
  globals::MATERIAL_MANAGER->bindMaterial(m_matHandle,
                                          SHADER_QUEUE_FLAGS::CUSTOM);


  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);

  /*
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
      inputDepthHandle, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      inputRTHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(inputRTHandle, inputDepthHandle);
  commandList->SetGraphicsRootSignature(rs);

  TextureHandle skyHandle = dx12::RENDERING_CONTEXT->getEnviromentMapHandle();
  // TextureHandle skyHandle =
  // globals::RENDERING_CONTEX->getEnviromentMapIrradianceHandle();
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  // commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  commandList->SetGraphicsRootDescriptorTable(
      1, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  // commandList->DrawInstanced(6, 1, 0, 0);
  // dx12::MESH_MANAGER->bindMeshRuntimeAndRender(skyboxHandle, currentFc);
  dx12::MESH_MANAGER->bindMesh(skyboxHandle, currentFc->commandList,
                               MeshAttributeFlags::POSITIONS, 2);
  dx12::MESH_MANAGER->render(skyboxHandle, currentFc);

  // reset normal viewport
  commandList->RSSetViewports(1, currViewport);
  annotateGraphicsEnd();
  */
}

void SkyBoxPass::onResizeEvent(int, int) {
  clear();
  initialize();
}

void SkyBoxPass::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  inputDepthHandle = getInputConnection<TextureHandle>(m_inConnections, DEPTH);

  // setting the render target output handle
  m_outputPlugs[0].plugValue = inputRTHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = inputRTHandle;
  bindings.colorRT[0].clearColor = {};
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = inputDepthHandle;
  bindings.depthStencil.clearDepthColor = {};
  bindings.depthStencil.clearStencilColor = {};
  bindings.depthStencil.shouldClearDepth = false;
  bindings.depthStencil.shouldClearStencil = false;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle =
      globals::RENDERING_CONTEXT->prepareBindingObject(bindings, "Skybox");

  TextureHandle skyHandle = dx12::RENDERING_CONTEXT->getEnviromentMapHandle();
  assert(skyHandle.isHandleValid());
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, skyHandle, 1,
                                         SHADER_QUEUE_FLAGS::CUSTOM);
}

void SkyBoxPass::clear() {
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
}  // namespace SirEngine
